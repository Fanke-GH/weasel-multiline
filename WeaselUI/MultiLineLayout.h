#pragma once

#include "StandardLayout.h"

namespace weasel {
// 多行候选布局: 候选项自左向右排列, 超过每行个数或行宽时换行,
// 视觉效果接近 macOS 拼音输入法的候选窗。
// 只负责几何计算, 不涉及任何输入逻辑。
class MultiLineLayout : public StandardLayout {
 public:
  MultiLineLayout(const UIStyle& style,
                  const Context& context,
                  const Status& status,
                  an<D2D>& pD2D)
      : StandardLayout(style, context, status, pD2D) {}
  virtual void DoLayout();

 private:
  // 单元格宽度: 自然宽度 + 高亮候选预留的 mark 位置
  struct Cell {
    int width;
    int height;
    bool has_comment;
  };
  std::vector<Cell> _MeasureCells(int base_offset) const;
  // 返回计算得到的行信息
  struct Rows {
    int count = 0;
    int per_row = 0;
    std::vector<int> row_of;  // 每个候选所在行
    std::vector<int> col_of;  // 每个候选所在列(行内序号)
    std::vector<int> x_of;    // 每个候选相对候选区左边的 x
    std::vector<int> col_w;   // 各列宽度(仅对齐模式有效)
    std::vector<int> top;     // 各行顶部 y
    std::vector<int> height;  // 各行高度
    std::vector<int> right;   // 各行最右边界(绝对坐标)
  };
  Rows _RowsByCount(const std::vector<Cell>& cells,
                    int per_row,
                    bool align_columns) const;
  Rows _RowsByWidth(const std::vector<Cell>& cells, int limit) const;
  void _CompleteRows(Rows& rows,
                     const std::vector<Cell>& cells,
                     int area_left,
                     int area_top,
                     int row_gap) const;
};
};  // namespace weasel
