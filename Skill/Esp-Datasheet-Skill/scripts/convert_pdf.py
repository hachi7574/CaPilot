#!/usr/bin/env python3
"""
convert_pdf.py — 芯片手册 PDF → 结构化 Markdown 转换工具

属于 Esp-Datasheet-Skill。使用 Marker 引擎将 PDF 转为 LLM 友好的 Markdown，
保留标题层级、表格（GFM）、LaTeX 公式（MathJax）、图片（含 alt）、Caption、中文文本。

同时整合了旧 ESP-PDF-SKILL 的 raw-pdf 模式（直接读 PDF），作为 fallback。

Usage:
    # 转换单个 PDF → Markdown
    python convert_pdf.py convert <pdf_path> [--output-dir <dir>]

    # 转换所有芯片手册
    python convert_pdf.py convert --all

    # === raw-pdf 模式（fallback，合并自旧 pdf_reader.py）===
    python convert_pdf.py info <pdf>              # PDF 基本信息
    python convert_pdf.py toc <pdf>               # 目录
    python convert_pdf.py search <pdf> <keyword>  # 搜索关键词
    python convert_pdf.py page <pdf> <n>          # 提取单页文本
    python convert_pdf.py pages <pdf> <start> [end]  # 提取页范围
    python convert_pdf.py tables <pdf> <n>        # 提取表格

Requires:
    marker-pdf      (pip install marker-pdf)
    pymupdf         (pip install pymupdf)      — raw-pdf 模式
"""

import sys
import os
import argparse
import pathlib

# === 路径常量 ==========================================================
# datasheets 目录：转换后的 Markdown 存放处
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
SKILL_ROOT = SCRIPT_DIR.parent
DATASHEETS_DIR = SKILL_ROOT / "datasheets"

# 默认手册源目录（相对于项目根）
PROJECT_ROOT = SKILL_ROOT.parent.parent
MANUALS_DIR = PROJECT_ROOT / "Hardware" / "Lceda-Esp32-S3-Practical-Board" / "Docs" / "02-Chip-Manuals"


# ======================================================================
# Marker 引擎转换
# ======================================================================

def convert_with_marker(pdf_path, output_dir, force=False):
    """使用 Marker 将 PDF 转为 Markdown，保留表格/公式/图片。"""
    try:
        from marker.converters.pdf import PdfConverter
        from marker.models import create_model_dict
    except ImportError:
        print("ERROR: marker-pdf not installed. Run: pip install marker-pdf", file=sys.stderr)
        sys.exit(1)

    pdf_path = pathlib.Path(pdf_path).resolve()
    chip_name = pdf_path.stem  # 如 QMI8658A

    # 输出目录: datasheets/{chip_name}/
    chip_dir = output_dir / chip_name

    # 跳过已转换的（除非 --force）
    if not force and (chip_dir / "full.md").exists():
        print(f"[SKIP] {chip_name} already converted (use --force to re-convert)")
        return None

    chip_dir.mkdir(parents=True, exist_ok=True)
    images_dir = chip_dir / "images"
    images_dir.mkdir(exist_ok=True)

    print(f"[Marker] Converting: {pdf_path.name}")
    print(f"  Output: {chip_dir}")

    # Marker v1.x API: PdfConverter(artifact_dict, config)
    # extract_images defaults to True, OUTPUT_IMAGE_FORMAT defaults to JPEG
    converter = PdfConverter(
        artifact_dict=create_model_dict(),
        config={}
    )

    rendered = converter(str(pdf_path))

    # MarkdownOutput fields: markdown, images, metadata
    markdown_text = rendered.markdown
    images_dict = rendered.images or {}  # {image_name: PIL.Image}
    meta = rendered.metadata or {}

    # 保存图片到 images/ 子目录
    img_count = 0
    for img_name, pil_img in images_dict.items():
        img_path = images_dir / img_name
        # PIL Image → save as JPEG (Marker default format)
        fmt = "JPEG" if img_name.lower().endswith(('.jpg', '.jpeg')) else "PNG"
        pil_img.save(str(img_path), format=fmt)
        img_count += 1
        # 更新 markdown 中的图片引用路径: image_name → images/image_name
        markdown_text = markdown_text.replace(
            f"]({img_name})",
            f"](images/{img_name})"
        )

    # 保存完整 Markdown
    md_file = chip_dir / "full.md"
    md_file.write_text(markdown_text, encoding="utf-8")

    # 提取 PDF 页数（从 metadata）
    page_count = meta.get("page_count", "N/A") if isinstance(meta, dict) else "N/A"

    # 生成 README.md（元数据）
    readme = f"""# {chip_name} — 芯片手册（Markdown 版）

## 元数据

| 项目 | 值 |
|------|-----|
| 源文件 | `{pdf_path.name}` |
| 转换引擎 | Marker (marker-pdf) |
| 转换时间 | {__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M')} |
| Markdown 文件 | `full.md` |
| 图片目录 | `images/` ({img_count} 张) |
| PDF 页数 | {page_count} |

## 使用方式

```bash
# 搜索关键词
python convert_pdf.py search-md {chip_name} "CTRL2"

# 查看完整文档
cat {chip_name}/full.md
```
"""
    (chip_dir / "README.md").write_text(readme, encoding="utf-8")

    print(f"  Markdown: {md_file} ({len(markdown_text):,} chars)")
    print(f"  Images:   {img_count}")
    print(f"  Done.")

    return chip_dir


def convert_all(output_dir, force=False):
    """转换手册目录下所有 PDF。"""
    if not MANUALS_DIR.exists():
        print(f"ERROR: manuals dir not found: {MANUALS_DIR}", file=sys.stderr)
        sys.exit(1)

    # 收集所有 PDF（去重，Windows 上 *.pdf 和 *.PDF 大小写不敏感会重复）
    pdfs = set(MANUALS_DIR.glob("*.pdf")) | set(MANUALS_DIR.glob("*.PDF"))
    pdfs = sorted(pdfs, key=lambda p: p.name.lower())
    if not pdfs:
        print(f"No PDF files found in {MANUALS_DIR}")
        return

    print(f"Found {len(pdfs)} PDF manuals to convert:\n")
    for p in pdfs:
        print(f"  {p.name}  ({p.stat().st_size / 1024:.0f} KB)")
    print()

    converted = 0
    failed = 0
    skipped = 0
    for pdf in pdfs:
        try:
            result = convert_with_marker(pdf, output_dir, force=force)
            if result is None:
                skipped += 1
            else:
                converted += 1
        except Exception as e:
            print(f"  FAILED: {e}", file=sys.stderr)
            failed += 1

    print(f"\n=== Summary: {converted} converted, {skipped} skipped, {failed} failed ===")

    # 转换完成后自动构建索引
    build_index(output_dir)


# ======================================================================
# Markdown 检索（从已转换的 Markdown 中搜索）
# ======================================================================

def search_markdown(chip_name, keyword):
    """在已转换的 Markdown 中搜索关键词。"""
    chip_dir = DATASHEETS_DIR / chip_name
    md_file = chip_dir / "full.md"

    if not md_file.exists():
        print(f"ERROR: {chip_name} not converted yet. Run: convert_pdf.py convert <pdf>", file=sys.stderr)
        # 提供回退建议
        print(f"  Fallback: use 'convert_pdf.py search <pdf> {keyword}' for raw PDF search", file=sys.stderr)
        sys.exit(1)

    content = md_file.read_text(encoding="utf-8")
    lines = content.split('\n')
    kw_lower = keyword.lower()

    found = 0
    for i, line in enumerate(lines):
        if kw_lower in line.lower():
            found += 1
            print(f"\n{'='*60}")
            print(f"Match #{found} (line {i+1}):")
            print(f"{'='*60}")
            start = max(0, i - 2)
            end = min(len(lines), i + 5)
            for j in range(start, end):
                marker = ">>>" if j == i else "   "
                print(f"{marker} {lines[j]}")

    if found == 0:
        print(f"No matches for '{keyword}' in {chip_name}/full.md")
    else:
        print(f"\n{found} match(es) found.")


# ======================================================================
# 索引构建
# ======================================================================

def build_index(output_dir):
    """构建 datasheets/_index.md 总索引。"""
    index_file = output_dir / "_index.md"

    chips = []
    for d in sorted(output_dir.iterdir()):
        if d.is_dir() and (d / "full.md").exists():
            md_size = (d / "full.md").stat().st_size
            img_count = len(list((d / "images").glob("*"))) if (d / "images").exists() else 0
            chips.append({
                "name": d.name,
                "md_size": md_size,
                "img_count": img_count,
            })

    lines = [
        "# 芯片手册索引（Markdown 版）",
        "",
        "由 `convert_pdf.py` 自动生成。请勿手动编辑。",
        "",
        f"生成时间：{__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M')}",
        "",
        "| 芯片 | Markdown | 大小 | 图片数 | 源 PDF |",
        "|------|----------|------|--------|--------|",
    ]

    for chip in chips:
        size_str = f"{chip['md_size'] / 1024:.0f} KB"
        lines.append(f"| [{chip['name']}]({chip['name']}/README.md) | [{chip['name']}/full.md]({chip['name']}/full.md) | {size_str} | {chip['img_count']} | `{chip['name']}.pdf` |")

    lines.append("")
    lines.append("## 检索方式")
    lines.append("")
    lines.append("```bash")
    lines.append('# 搜索某芯片中的关键词')
    lines.append('python convert_pdf.py search-md <chip_name> "CTRL2"')
    lines.append("")
    lines.append("# raw-pdf 模式（直接读 PDF，作为 fallback）")
    lines.append('python convert_pdf.py search <pdf> "CTRL2"')
    lines.append("```")

    index_file.write_text('\n'.join(lines), encoding="utf-8")
    print(f"Index updated: {index_file} ({len(chips)} chips)")


# ======================================================================
# raw-pdf 模式（合并自旧 pdf_reader.py，作为 fallback）
# ======================================================================

def raw_pdf_info(doc, pdf_path):
    """PDF 基本信息。"""
    print(f"File:     {pdf_path}")
    print(f"Pages:    {doc.page_count}")
    meta = doc.metadata
    if meta:
        print(f"Title:    {meta.get('title', 'N/A')}")
        print(f"Author:   {meta.get('author', 'N/A')}")
    toc = doc.get_toc()
    print(f"TOC:      {len(toc)} entries" if toc else "TOC:      (none)")
    print()
    print("--- Page previews (first 3 pages) ---")
    for i in range(min(3, doc.page_count)):
        text = doc[i].get_text().strip()
        preview = text[:200].replace('\n', ' ')
        print(f"  Page {i+1}: {preview}...")


def raw_pdf_toc(doc):
    """PDF 目录。"""
    toc = doc.get_toc()
    if not toc:
        print("(No table of contents / bookmarks in this PDF)")
        return
    for level, title, page in toc:
        indent = "  " * (level - 1)
        print(f"{indent}{title}  (p.{page})")


def raw_pdf_search(doc, keyword):
    """原始 PDF 搜索。"""
    kw_lower = keyword.lower()
    found = 0
    for i in range(doc.page_count):
        text = doc[i].get_text()
        if kw_lower in text.lower():
            found += 1
            print(f"\n{'='*60}")
            print(f"Page {i+1} (match #{found}):")
            print(f"{'='*60}")
            lines = text.split('\n')
            for j, line in enumerate(lines):
                if kw_lower in line.lower():
                    start = max(0, j - 1)
                    end = min(len(lines), j + 3)
                    for k in range(start, end):
                        marker = ">>>" if k == j else "   "
                        print(f"{marker} {lines[k]}")
                    print("   ...")
    if found == 0:
        print(f"No matches found for '{keyword}'")


def raw_pdf_pages(doc, start, end=None):
    """提取页范围文本。"""
    if end is None:
        end = start
    start_idx = max(0, start - 1)
    end_idx = min(doc.page_count, end)
    for i in range(start_idx, end_idx):
        text = doc[i].get_text()
        print(f"\n{'='*60}")
        print(f"Page {i+1}")
        print(f"{'='*60}")
        print(text)


def raw_pdf_tables(doc, page_num):
    """提取表格。"""
    page_idx = page_num - 1
    if page_idx < 0 or page_idx >= doc.page_count:
        print(f"Error: page {page_num} out of range (1-{doc.page_count})")
        return
    page = doc[page_idx]
    tables = page.find_tables()
    if tables and tables.tables:
        for ti, table in enumerate(tables.tables):
            print(f"\n--- Table {ti+1} on page {page_num} ---")
            rows = table.extract()
            for row in rows:
                cells = [str(c).strip() if c else "" for c in row]
                print(" | ".join(cells))
    else:
        print(f"No structured tables found on page {page_num}.")
        print("Showing raw text:")
        print("-" * 60)
        print(page.get_text())


def raw_pdf_mode(pdf_path, command, *args):
    """raw-pdf 模式入口（合并自旧 pdf_reader.py）。"""
    try:
        import fitz
    except ImportError:
        print("ERROR: pymupdf not installed. Run: pip install pymupdf", file=sys.stderr)
        sys.exit(1)

    try:
        doc = fitz.open(pdf_path)
    except Exception as e:
        print(f"Error opening PDF: {e}", file=sys.stderr)
        sys.exit(1)

    if command == "info":
        raw_pdf_info(doc, pdf_path)
    elif command == "toc":
        raw_pdf_toc(doc)
    elif command == "search":
        if not args:
            print("Error: search requires a keyword")
            sys.exit(1)
        raw_pdf_search(doc, args[0])
    elif command == "pages":
        if not args:
            print("Error: pages requires a start page number")
            sys.exit(1)
        start = int(args[0])
        end = int(args[1]) if len(args) > 1 else None
        raw_pdf_pages(doc, start, end)
    elif command == "page":
        if not args:
            print("Error: page requires a page number")
            sys.exit(1)
        raw_pdf_pages(doc, int(args[0]))
    elif command == "tables":
        if not args:
            print("Error: tables requires a page number")
            sys.exit(1)
        raw_pdf_tables(doc, int(args[0]))
    else:
        print(f"Unknown raw-pdf command: {command}")
        sys.exit(1)

    doc.close()


# ======================================================================
# 主入口
# ======================================================================

def main():
    parser = argparse.ArgumentParser(
        description="芯片手册 PDF → Markdown 转换 + 检索工具",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    sub = parser.add_subparsers(dest="command", required=True)

    # convert
    p_conv = sub.add_parser("convert", help="转换 PDF → Markdown (Marker 引擎)")
    p_conv.add_argument("pdf", nargs="?", help="PDF 文件路径（省略时配合 --all）")
    p_conv.add_argument("--all", action="store_true", help="转换所有手册")
    p_conv.add_argument("--force", action="store_true", help="强制重新转换（跳过已转换的）")
    p_conv.add_argument("--output-dir", default=str(DATASHEETS_DIR), help="输出目录")

    # search-md
    p_smd = sub.add_parser("search-md", help="在已转换的 Markdown 中搜索")
    p_smd.add_argument("chip", help="芯片名（如 QMI8658A）")
    p_smd.add_argument("keyword", help="搜索关键词")

    # build-index
    sub.add_parser("build-index", help="重建索引")

    # raw-pdf 模式
    p_info = sub.add_parser("info", help="[raw-pdf] PDF 基本信息")
    p_info.add_argument("pdf")

    p_toc = sub.add_parser("toc", help="[raw-pdf] PDF 目录")
    p_toc.add_argument("pdf")

    p_search = sub.add_parser("search", help="[raw-pdf] 原始 PDF 搜索")
    p_search.add_argument("pdf")
    p_search.add_argument("keyword")

    p_page = sub.add_parser("page", help="[raw-pdf] 提取单页")
    p_page.add_argument("pdf")
    p_page.add_argument("page_num", type=int)

    p_pages = sub.add_parser("pages", help="[raw-pdf] 提取页范围")
    p_pages.add_argument("pdf")
    p_pages.add_argument("start", type=int)
    p_pages.add_argument("end", type=int, nargs="?", default=None)

    p_tables = sub.add_parser("tables", help="[raw-pdf] 提取表格")
    p_tables.add_argument("pdf")
    p_tables.add_argument("page_num", type=int)

    args = parser.parse_args()

    if args.command == "convert":
        output_dir = pathlib.Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        if args.all:
            convert_all(output_dir, force=args.force)
        elif args.pdf:
            convert_with_marker(args.pdf, output_dir, force=args.force)
            build_index(output_dir)
        else:
            parser.error("convert requires <pdf> or --all")

    elif args.command == "search-md":
        search_markdown(args.chip, args.keyword)

    elif args.command == "build-index":
        build_index(DATASHEETS_DIR)

    elif args.command in ("info", "toc", "search", "page", "pages", "tables"):
        # raw-pdf 模式
        if args.command == "info":
            raw_pdf_mode(args.pdf, "info")
        elif args.command == "toc":
            raw_pdf_mode(args.pdf, "toc")
        elif args.command == "search":
            raw_pdf_mode(args.pdf, "search", args.keyword)
        elif args.command == "page":
            raw_pdf_mode(args.pdf, "page", str(args.page_num))
        elif args.command == "pages":
            extra = [str(args.end)] if args.end else []
            raw_pdf_mode(args.pdf, "pages", str(args.start), *extra)
        elif args.command == "tables":
            raw_pdf_mode(args.pdf, "tables", str(args.page_num))


if __name__ == "__main__":
    main()
