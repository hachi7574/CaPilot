# 芯片手册索引（Markdown 版）

由 `convert_pdf.py` 自动生成。请勿手动编辑。

生成时间：2026-07-05 19:02

| 芯片 | Markdown | 大小 | 图片数 | 源 PDF |
|------|----------|------|--------|--------|
| [CH340K](CH340K/README.md) | [CH340K/full.md](CH340K/full.md) | 34 KB | 12 | `CH340K.pdf` |

## 检索方式

```bash
# 搜索某芯片中的关键词
python convert_pdf.py search-md <chip_name> "CTRL2"

# raw-pdf 模式（直接读 PDF，作为 fallback）
python convert_pdf.py search <pdf> "CTRL2"
```