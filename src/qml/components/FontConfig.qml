pragma Singleton
import QtQuick 2.15

/**
 * 全局字体大小配置
 * 
 * 使用方法：在任何QML文件中通过 FontConfig.xxx 访问字体大小
 * 例如：font.pixelSize: FontConfig.normal
 * 
 * 档位说明：
 *   tiny    - 最小字体，用于次要标签、脚注
 *   small   - 小字体，用于辅助信息、提示
 *   normal  - 正常字体，用于普通文本
 *   medium  - 中等字体，用于重要文本、按钮
 *   large   - 大字体，用于标题、强调
 *   xlarge  - 超大字体，用于主标题
 *   huge    - 巨大字体，用于数值显示、图标
 * 
 * 调整方法：修改 scaleFactor 可以整体缩放所有字体
 *          或单独修改各档位的基础值
 */
QtObject {
    id: fontConfig
    
    // ==================== 全局缩放因子 ====================
    // 修改此值可整体调整所有字体大小
    // 1.0 = 默认大小, 1.2 = 放大20%, 0.8 = 缩小20%
    readonly property real scaleFactor: 1.1
    
    // ==================== 字体档位定义 ====================
    // 基础字体大小（未缩放前的值）
    readonly property int _baseTiny: 10      // 原 8-9px
    readonly property int _baseSmall: 12     // 原 10-11px
    readonly property int _baseNormal: 14    // 原 12-13px
    readonly property int _baseMedium: 16    // 原 14-16px
    readonly property int _baseLarge: 20     // 原 18-20px
    readonly property int _baseXLarge: 24    // 原 22-24px
    readonly property int _baseHuge: 36      // 原 32-40px
    
    // ==================== 实际使用的字体大小 ====================
    // 最小字体 - 用于次要标签、脚注、单位等
    readonly property int tiny: Math.round(_baseTiny * scaleFactor)
    
    // 小字体 - 用于辅助信息、提示文字、状态文字
    readonly property int small: Math.round(_baseSmall * scaleFactor)
    
    // 正常字体 - 用于普通文本、列表项
    readonly property int normal: Math.round(_baseNormal * scaleFactor)
    
    // 中等字体 - 用于重要文本、按钮文字、面板标签
    readonly property int medium: Math.round(_baseMedium * scaleFactor)
    
    // 大字体 - 用于标题、强调文字
    readonly property int large: Math.round(_baseLarge * scaleFactor)
    
    // 超大字体 - 用于主标题、对话框标题
    readonly property int xlarge: Math.round(_baseXLarge * scaleFactor)
    
    // 巨大字体 - 用于数值显示、大图标
    readonly property int huge: Math.round(_baseHuge * scaleFactor)
    
    // ==================== 特殊用途字体大小 ====================
    // 图标字体大小
    readonly property int iconSmall: Math.round(14 * scaleFactor)
    readonly property int iconNormal: Math.round(18 * scaleFactor)
    readonly property int iconLarge: Math.round(24 * scaleFactor)
    
    // 数值显示专用
    readonly property int valueDisplay: Math.round(40 * scaleFactor)
}
