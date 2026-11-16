#pragma once


#include "FootprintItem.hpp"
#include "GeometryTypes.hpp"
/*   
    约定：
    坐标全部使用封装局部坐标，单位 mm
    旋转角度rotation、startAngle、sweepAngle 统一使用角度制
    多边形points按边界顺序排列，绘制时自动首尾闭合，不重复存首点
    命中检测tolerance使用 mm；线段、矩形、多边形等根据真实几何进行检测
*/
class LineItem : public GraphicItem //线的类
{
public:
    ItemType GetType() const override { return ItemType::Line; } //返回的类型, 不用重写


    //基本的参数
    Point2D start; //起点位置
    Point2D end; //终点位置
    double linewidth =0.15;//线的宽度
    std::unique_ptr<FootprintItem> Clone() const override; //用于复制撤销, 需要重写
    void Translate(double dx, double dy) override; //相对原点的位置

    Bounds GetBounds()const; //返回线的边界(矩形)

    bool HitTest(double x, double y, double tolerance) const override; //判断鼠标点击的点是否在线上, 用于鼠标选择线使用, 需要重写
    // 前两个参数判断鼠标的坐标, tolerance是容差, 也就是鼠标点击的点离线的距离小于tolerance就认为是点击了线
    bool Validate(std::string* error = nullptr) const override; //一个安全测试函数
    // 判断这些数据是否合法, 用于后续使用, 比如这个宽度高度不小于零这种
};
class RectItem : public GraphicItem //矩形的基类
{
public:
    ItemType GetType() const override { return ItemType::Rect; } //返回的类型, 不用重写


    //基本的参数
    Point2D centre;//中心位置
    double width = 0.1;//宽度
    double height = 0.1;//高度
    double rotation = 0.0; //旋转角度
    double linewidth =0.15;//线的宽度
    

    std::unique_ptr<FootprintItem> Clone() const override; //用于复制撤销, 需要重写
    void Translate(double dx, double dy) override; //平移矩形

    Bounds GetBounds() const; //返回矩形4的边界(矩形)

    bool HitTest(double x, double y, double tolerance) const override; //判断鼠标点击的点是否在矩形上, 用于鼠标选择矩形使用, 需要重写
    // 前两个参数判断鼠标的坐标, tolerance是容差, 也就是鼠标点击的点离矩形的距离小于tolerance就认为是点击了矩形


    bool Validate(std::string* error = nullptr) const override; //一个安全测试函数
    // 判断这些数据是否合法, 用于后续使用, 比如这个宽度高度不小于零这种
};
class ArcItem : public GraphicItem //弧的基类 
{
public:
    ItemType GetType() const override { return ItemType::Arc; } //返回的类型, 不用重写


    //基本的参数
    Point2D centre;//中心位置
    double radiusX = 1.0;//半径X
    double radiusY = 1.0;//半径Y

    double startAngle = 0.0;//起始角度
    double sweepAngle = 90.0;//扫过的角度
    double rotation = 0.0;//旋转角度
    double lineWidth = 0.155;//线的宽度

    std::unique_ptr<FootprintItem> Clone() const override; //用于复制撤销, 需要重写
    void Translate(double dx, double dy) override; //弧平移

    Bounds GetBounds() const; //返回弧的边界(矩形), 

    bool HitTest(double x, double y, double tolerance) const override; //判断鼠标点击的点是否在弧上, 用于鼠标选择弧使用, 需要重写
    // 前两个参数判断鼠标的坐标, tolerance是容差, 也就是鼠标点击的点离弧的距离小于tolerance就认为是点击了弧


    bool Validate(std::string* error = nullptr) const override; //一个安全测试函数
    // 判断这些数据是否合法, 用于后续使用, 比如这个宽度高度不小于零这种
};
class PolygonItem : public GraphicItem //多边形
{
public:
    ItemType GetType() const override { return ItemType::Polygon; } //返回的类型, 不用重写

    //基本的参数
    std::vector<Point2D> points; //多边形的点集合
    double lineWidth = 0.15; //线的宽度
    bool filled = false;
    std::unique_ptr<FootprintItem> Clone() const override; //用于复制撤销, 需要重写
    void Translate(double dx, double dy) override; //相对原点的位置

    Bounds GetBounds()const; //返回多边形的边界(矩形), 用于绘制使用

    bool HitTest(double x, double y, double tolerance) const override; //判断鼠标点击的点是否在多边形上, 用于鼠标选择多边形使用, 需要重写
    // 前两个参数判断鼠标的坐标, tolerance是容差, 也就是鼠标点击的点离多边形的距离小于tolerance就认为是点击了多边形


    bool Validate(std::string* error = nullptr) const override; //一个安全测试函数
    // 判断这些数据是否合法, 用于后续使用, 比如这个宽度高度不小于零这种
};
class TextItem : public GraphicItem //文本项
{
public:
    ItemType GetType() const override { return ItemType::Text; } //返回的类型, 不用重写

    //基本的参数
    Point2D position; //文本中心
    double height = 1.0; //文本高度
    double width = 1.0; //单个字符的名义宽度
    double rotation = 0.0; //旋转角度
    double strokeWidth = 0.15; //笔画宽度
    std::string text; //文本内容    
    
    std::unique_ptr<FootprintItem> Clone() const override; //用于复制撤销, 需要重写
    void Translate(double dx, double dy) override; //相对原点的位置

    Bounds GetBounds()const; //返回文本的边界(矩形), 用于绘制使用

    bool HitTest(double x, double y, double tolerance) const override; //判断鼠标点击的点是否在文本上, 用于鼠标选择文本使用, 需要重写
    // 前两个参数判断鼠标的坐标, tolerance是容差, 也就是鼠标点击的点离文本的距离小于tolerance就认为是点击了文本

    bool Validate(std::string* error = nullptr) const override;//一个安全测试函数
    // 判断这些数据是否合法, 用于后续使用, 比如这个宽度高度不小于零这种
};