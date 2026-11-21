#include "MultiLineLayout.h"
#include <vector>

using namespace weasel;

// 计算每个候选所占单元格的宽度/高度/是否有注释
// 与 HorizontalLayout 的算法保持一致: 序号与文字之间固定插入 hilite_spacing,
// 有注释时再插入一个 hilite_spacing。
// base_offset 为高亮候选前 mark(竖线/mark_text) 占位宽度, 计入单元格宽度。
std::vector<MultiLineLayout::Cell> MultiLineLayout::_MeasureCells(
    int base_offset) const {
  std::vector<Cell> cells;
  cells.reserve(candidates_count);
  for (auto i = 0; i < candidates_count; ++i) {
    const auto& labelSize = _candidateLabelSizes[i];
    const auto& textSize = _candidateTextSizes[i];
    const auto& commentSize = _candidateCommentSizes[i];
    Cell cell;
    cell.width = labelSize.cx + _style.hilite_spacing + textSize.cx;
    cell.height = MAX(labelSize.cy, textSize.cy);
    cell.has_comment = false;
    bool cmtFontNotTrans =
        (i == id && (_style.hilited_comment_text_color & 0xff000000)) ||
        (i != id && (_style.comment_text_color & 0xff000000));
    if (!comments.at(i).str.empty() && cmtFontValid && cmtFontNotTrans) {
      cell.width += _style.hilite_spacing + commentSize.cx;
      cell.height = MAX(cell.height, commentSize.cy);
      cell.has_comment = true;
    }
    if (i == id)
      cell.width += base_offset;
    if (cell.height <= 0)
      cell.height = textSize.cy;
    cells.push_back(cell);
  }
  return cells;
}

// 每行固定 per_row 个候选
MultiLineLayout::Rows MultiLineLayout::_RowsByCount(
    const std::vector<Cell>& cells,
    int per_row,
    bool align_columns) const {
  Rows rows;
  if (per_row <= 0 || cells.empty())
    return rows;
  rows.per_row = per_row;
  rows.count = ((int)cells.size() + per_row - 1) / per_row;
  rows.row_of.assign(cells.size(), 0);
  rows.col_of.assign(cells.size(), 0);
  rows.x_of.assign(cells.size(), 0);
  // 仅对齐模式使用 col_w; 非对齐模式下保持为空, 由调用方据此判断
  if (align_columns)
    rows.col_w.assign(per_row, 0);
  for (size_t i = 0; i < cells.size(); ++i) {
    rows.row_of[i] = (int)i / per_row;
    rows.col_of[i] = (int)i % per_row;
    if (align_columns)
      rows.col_w[rows.col_of[i]] =
          MAX(rows.col_w[rows.col_of[i]], cells[i].width);
  }
  int x = 0;
  for (size_t i = 0; i < cells.size(); ++i) {
    if (rows.col_of[i] == 0)
      x = 0;
    rows.x_of[i] = x;
    const int w = align_columns ? rows.col_w[rows.col_of[i]] : cells[i].width;
    x += w + _style.candidate_spacing;
  }
  return rows;
}

// 按可用宽度 limit 贪心换行
MultiLineLayout::Rows MultiLineLayout::_RowsByWidth(
    const std::vector<Cell>& cells,
    int limit) const {
  Rows rows;
  if (cells.empty())
    return rows;
  rows.row_of.assign(cells.size(), 0);
  rows.col_of.assign(cells.size(), 0);
  rows.x_of.assign(cells.size(), 0);
  int count = 1, x = 0, col = 0, max_col = 0;
  for (size_t i = 0; i < cells.size(); ++i) {
    if (i > 0) {
      if (limit > 0 && x + _style.candidate_spacing + cells[i].width > limit) {
        ++count;
        x = 0;
        col = 0;
      } else {
        x += _style.candidate_spacing;
        ++col;
      }
    }
    rows.row_of[i] = count - 1;
    rows.col_of[i] = col;
    rows.x_of[i] = x;
    max_col = MAX(max_col, col);
    x += cells[i].width;
  }
  rows.count = count;
  rows.per_row = max_col + 1;
  return rows;
}

// 计算各行高度、顶部位置与最右边界
void MultiLineLayout::_CompleteRows(Rows& rows,
                                    const std::vector<Cell>& cells,
                                    int area_left,
                                    int area_top,
                                    int row_gap) const {
  if (rows.count <= 0)
    return;
  const bool align_columns = !rows.col_w.empty();
  rows.height.assign(rows.count, 0);
  rows.top.assign(rows.count, 0);
  rows.right.assign(rows.count, 0);
  for (size_t i = 0; i < cells.size(); ++i) {
    const int r = rows.row_of[i];
    rows.height[r] = MAX(rows.height[r], cells[i].height);
    const int w = align_columns ? rows.col_w[rows.col_of[i]] : cells[i].width;
    rows.right[r] = MAX(rows.right[r], area_left + rows.x_of[i] + w);
  }
  rows.top[0] = area_top;
  for (auto r = 1; r < rows.count; ++r)
    rows.top[r] = rows.top[r - 1] + rows.height[r - 1] + row_gap;
}

void MultiLineLayout::DoLayout() {
  int width = offsetX + real_margin_x, height = offsetY + real_margin_y;
  int w = offsetX + real_margin_x;

  // 高亮候选前的 mark 占位
  int base_offset = _CalcMarkMetrics(false);

  // 翻页指示符尺寸
  int pgw = 0, pgh = 0;
  _CalcPageIndicator(false, pgw, pgh);
  // 行焦点多行模式不需要 "< >" 翻页指示符, 连预留宽度一并去掉
  if (_style.multiline_row_focus &&
      _style.layout_type == UIStyle::LAYOUT_HORIZONTAL_MULTILINE) {
    pgw = 0;
    pgh = 0;
  }

  /* Preedit */
  if (!IsInlinePreedit() && !_context.preedit.str.empty()) {
    _LayoutInlineRect(_preeditSize, false, true, pgw, pgh, w, width, height,
                      _preeditRect);
  }

  /* Auxiliary */
  if (!_context.aux.str.empty()) {
    _LayoutInlineRect(_auxSize, false, false, pgw, pgh, w, width, height,
                      _auxiliaryRect);
  }

  const int area_left = offsetX + real_margin_x;
  const int area_top = height;
  // 翻页指示符排在候选区右侧, 需要预留宽度
  const bool show_page =
      _pageEnabled && candidates_count && !_style.inline_preedit && pgw > 0;
  const int page_reserve = show_page ? (pgw + _style.candidate_spacing) : 0;
  // 行间距不小于上下高亮内边距的两倍, 避免相邻行的高亮块粘连
  const int row_gap =
      MAX(_style.candidate_spacing, _style.hilite_padding_y * 2);

  Rows rows;

  if (candidates_count) {
    const std::vector<Cell> cells = _MeasureCells(base_offset);

    bool by_count = _style.multiline_count_per_row > 0;
    // 换行可用宽度, 与水平布局 style/layout/max_width 的语义一致
    int limit =
        (_style.max_width > 0)
            ? (_style.max_width - offsetX - real_margin_x * 2 - page_reserve)
            : 0;
    if (limit < 0)
      limit = 0;

    if (by_count) {
      int per_row = _style.multiline_count_per_row;
      if (_style.multiline_max_rows > 0) {
        // 行数上限优先: 保证不超过 max_rows 行
        const int min_per_row =
            (candidates_count + _style.multiline_max_rows - 1) /
            _style.multiline_max_rows;
        per_row = MAX(per_row, min_per_row);
      }
      rows = _RowsByCount(cells, per_row, _style.multiline_align_columns);
    } else if (limit > 0) {
      rows = _RowsByWidth(cells, limit);
      if (_style.multiline_max_rows > 0 &&
          rows.count > _style.multiline_max_rows) {
        // 按宽度换行会超出行数上限, 改为按个数均分
        const int per_row = (candidates_count + _style.multiline_max_rows - 1) /
                            _style.multiline_max_rows;
        rows = _RowsByCount(cells, per_row, _style.multiline_align_columns);
      }
    } else if (_style.multiline_max_rows > 0) {
      const int per_row = (candidates_count + _style.multiline_max_rows - 1) /
                          _style.multiline_max_rows;
      rows = _RowsByCount(cells, per_row, _style.multiline_align_columns);
    } else {
      // 未设置每行个数与行宽: 单行, 行为与 horizontal 布局相同
      rows = _RowsByCount(cells, candidates_count, false);
    }

    _CompleteRows(rows, cells, area_left, area_top, row_gap);

    /* 候选项矩形 */
    const bool align_columns = !rows.col_w.empty();
    for (auto i = 0; i < candidates_count; ++i) {
      const auto& labelSize = _candidateLabelSizes[i];
      const auto& textSize = _candidateTextSizes[i];
      const auto& commentSize = _candidateCommentSizes[i];
      const int r = rows.row_of[i];
      const int top = rows.top[r];
      const int h = rows.height[r];
      const int x = area_left + rows.x_of[i] + (i == id ? base_offset : 0);
      /* Label */
      _candidateLabelRects[i].SetRect(x, top, x + labelSize.cx,
                                      top + labelSize.cy);
      /* Text */
      _candidateTextRects[i].SetRect(
          x + labelSize.cx + _style.hilite_spacing, top,
          x + labelSize.cx + _style.hilite_spacing + textSize.cx,
          top + textSize.cy);
      /* Comment */
      const int cmt_left =
          x + labelSize.cx + _style.hilite_spacing + textSize.cx;
      if (cells[i].has_comment) {
        _candidateCommentRects[i].SetRect(
            cmt_left + _style.hilite_spacing, top,
            cmt_left + _style.hilite_spacing + commentSize.cx,
            top + commentSize.cy);
      } else {
        _candidateCommentRects[i].SetRect(cmt_left, top, cmt_left,
                                          top + textSize.cy);
      }
      // 行内垂直对齐
      if (_style.align_type != UIStyle::ALIGN_TOP) {
        int ol = 0, ot = 0, oc = 0;
        if (_style.align_type == UIStyle::ALIGN_CENTER) {
          ol = (h - _candidateLabelRects[i].Height()) / 2;
          ot = (h - _candidateTextRects[i].Height()) / 2;
          oc = (h - _candidateCommentRects[i].Height()) / 2;
        } else if (_style.align_type == UIStyle::ALIGN_BOTTOM) {
          ol = h - _candidateLabelRects[i].Height();
          ot = h - _candidateTextRects[i].Height();
          oc = h - _candidateCommentRects[i].Height();
        }
        _candidateLabelRects[i].OffsetRect(0, ol);
        _candidateTextRects[i].OffsetRect(0, ot);
        _candidateCommentRects[i].OffsetRect(0, oc);
      }
      // 整体矩形从单元格左边开始, 以便把高亮候选的 mark 位置包含进来
      const int left = area_left + rows.x_of[i];
      const int cell_right =
          left + (align_columns ? rows.col_w[rows.col_of[i]] : cells[i].width);
      _candidateRects[i].SetRect(
          left, top, MAX(_candidateCommentRects[i].right, cell_right), top + h);
    }

    int max_right = 0;
    for (auto r = 0; r < rows.count; ++r)
      max_right = MAX(max_right, rows.right[r]);
    // 内容右边界与 horizontal 布局一致(max_right 已含左边距, 右边距统一在
    // 后面的 width += real_margin_x 加入); 翻页指示符另占 page_reserve 宽度
    const int content_right = max_right;
    for (auto i = 0; i < candidates_count; ++i) {
      const bool last_in_row =
          (i == candidates_count - 1) || (rows.row_of[i + 1] != rows.row_of[i]);
      if (last_in_row && _candidateRects[i].right < content_right)
        _candidateRects[i].right = content_right;
    }
    width = MAX(width, max_right + page_reserve);
    height = rows.top[rows.count - 1] + rows.height[rows.count - 1] - offsetY;
  } else {
    height -= _style.spacing + offsetY;
    width += _style.hilite_spacing + _style.border;
  }

  width += real_margin_x;
  height += real_margin_y;

  if (candidates_count) {
    width = MAX(width, _style.min_width);
    height = MAX(height, _style.min_height);
  }
  _highlightRect = _candidateRects[id];
  _UpdateStatusIconLayout(&width, &height);
  _contentSize.SetSize(width + offsetX, height + 2 * offsetY);
  _contentRect.SetRect(0, 0, _contentSize.cx, _contentSize.cy);

  // 翻页指示符: 位于候选区右侧, 垂直居中于所有候选行
  if (show_page && rows.count > 0) {
    const int _prex = _contentSize.cx - offsetX - real_margin_x +
                      _style.hilite_padding_x - pgw;
    const int area_bottom =
        rows.top[rows.count - 1] + rows.height[rows.count - 1];
    const int _prey = (area_top + area_bottom) / 2 - _pagePrevSize.cy / 2;
    _prePageRect.SetRect(_prex, _prey, _prex + _pagePrevSize.cx,
                         _prey + _pagePrevSize.cy);
    _nextPageRect.SetRect(
        _prePageRect.right + _style.hilite_spacing, _prey,
        _prePageRect.right + _style.hilite_spacing + _pageNextSize.cx,
        _prey + _pageNextSize.cy);
    if (ShouldDisplayStatusIcon()) {
      _prePageRect.OffsetRect(-STATUS_ICON_SIZE, 0);
      _nextPageRect.OffsetRect(-STATUS_ICON_SIZE, 0);
    }
  }

  // prepare temp rect _bgRect for roundinfo calculation
  CopyRect(_bgRect, _contentRect);
  _bgRect.DeflateRect(offsetX + 1, offsetY + 1);
  // 先按水平布局规则计算圆角, 再修正多行的情况
  _PrepareRoundInfo();
  if (rows.count > 1) {
    for (auto i = 0; i < candidates_count; ++i) {
      const int r = rows.row_of[i];
      const bool first_in_row = (i == 0) || (rows.row_of[i - 1] != r);
      const bool last_in_row =
          (i == candidates_count - 1) || (rows.row_of[i + 1] != r);
      const bool top_row = (r == 0);
      const bool bottom_row = (r == rows.count - 1);
      if (!_roundInfo[i].Hemispherical) {
        // 高亮没有贴到窗口边缘, 维持独立圆角小块(与默认非合并模式一致)
        _roundInfo[i] = IsToRoundStruct();
        continue;
      }
      // 贴边的高亮与窗口圆角合并: 只在整个网格的外角处画圆角
      _roundInfo[i].IsTopLeftNeedToRound = top_row && first_in_row;
      _roundInfo[i].IsBottomLeftNeedToRound = bottom_row && first_in_row;
      _roundInfo[i].IsTopRightNeedToRound = top_row && last_in_row;
      _roundInfo[i].IsBottomRightNeedToRound = bottom_row && last_in_row;
    }
  }

  // Precompute preedit sub-rectangles
  _PrecomputePreeditRects(_preeditRect, _context.preedit, _preeditBeforeRect,
                          _preeditHiliteRect, _preeditAfterRect);

  // Precompute auxiliary sub-rectangles
  _PrecomputePreeditRects(_auxiliaryRect, _context.aux, _auxBeforeRect,
                          _auxHiliteRect, _auxAfterRect);

  // truely draw content size calculation
  _contentRect.DeflateRect(offsetX, offsetY);
}
