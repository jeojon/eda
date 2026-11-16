// 本文件定义了所有的FootprintItem类的接口,用于表示PCB设计中的封装元件的外形和引脚信息
// 主要是定义了 封装的元件父类, 详细的子元件需要继承类中实现

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>

using std::string;
using std::vector;
//一些基本的枚举
enum class Layer // 表示层级, 一个铜层, 一个丝印层, 主要用于绘制和保存使用
{
    Copper, //铜
    SilkScreen //丝印
};

enum class ItemType //元件的类型
{
    Pad,        // 焊盘
    Line,       // 直线
    Rect,       // 矩形
    Arc,        // 圆弧
    Polygon,    // 多边形
    Text        // 文字
};

enum class PadKind //定义的焊盘的类型,决定焊盘是穿孔还是表面涂装
{
    Smd, //表面涂装
    ThroughHole //穿孔
};
enum class PadShape //定义焊盘的形状
{
    Rect,          // 矩形
    RoundedRect,   // 圆角矩形
    Circle         // 圆形
};
struct Bounds //这个结构体是用来表示元件的边界的外接矩形, 用于绘制和选择使用
{
    double minX = 0.0; //x最小值边界
    double minY = 0.0; //y最小值边界
    double maxX = 0.0; //x最大值边界
    double maxY = 0.0; //y最大值边界
    bool valid = false;
};



class FootprintItem //所有元件的父类, 用于存储一些基本的元件功能
{
public:
    virtual ~FootprintItem() = default; //虚析构, 主要是为了子类的析构函数能够被正确调用, 释放资源
    virtual ItemType GetType() const = 0; //获得元件的类型, 所以类型定义在一个枚举里
    virtual void Translate(double dx, double dy) = 0;  // 这个是元件的整体平移(相对于封装原点,上边有定义), 可以表示这个元件的位置
    virtual std::unique_ptr<FootprintItem> Clone() const = 0; //用于撤销服务和复制粘贴
    // 判断鼠标是否命中图元，具体算法由子类实现
    virtual bool HitTest(
        double x, double y, double tolerance
    ) const = 0;
    // 检查图元数据是否合法，失败时可返回错误原因
    virtual bool Validate(
        std::string* error = nullptr
    ) const = 0;
};

//定义整个封装
struct position //这个封装的位置坐标(原点位置)的结构体
{
    double x;
    double y;
};

class Footprint //封装类(保存一个封装的所有元件), 这个类只跟文件保存和计算有关, 绘图不用管
{
    public:
    //以下四个属性是封装的基本信息, 跟文件保存有关系, 但是跟元件绘制等无关
    string name; // 封装名称
    string category; // 封装类别
    string description; // 封装描述
    position pos; // 封装位置

    // 封装保存元件的部分
    vector<std::unique_ptr<FootprintItem>> items;  

    //构造函数
    Footprint() : name(""), category(""), description(""), pos({0.0, 0.0}) {}
};



class PadItem : public FootprintItem //焊盘和引脚的基类
{
public:
    ItemType GetType() const override { return ItemType::Pad; } //返回的类型, 不用重写

    PadKind kind = PadKind::Smd; //表示穿孔还是表面,具体的见枚举
    PadShape shape = PadShape::RoundedRect; //形状

    std::string number; //焊盘的编号, 决定了元件的引脚和电路板的连接关系
    std::string name; //焊盘的名称, 这个是元件的引脚名称, 主要用于元件的识别和绘制

    //基本的参数
    double x = 0.0; //中心位置
    double y = 0.0;
    double width = 1.0;
    double height = 1.0;
    double rotation = 0.0; //旋转角度

    double drill = 0.0; //表示钻孔的直径大小, 要是表面涂装就是0, 要是穿孔就是钻孔的直径大小
    double maskExpansion = 0.1; //这个是开窗大小,就是焊盘的外接矩形与实际焊盘的差值
    double clearance = 0.2; //安全距离, 也就是两个焊盘最小间距

    std::unique_ptr<FootprintItem> Clone() const override; //用于复制撤销, 需要重写
    void Translate(double dx, double dy) override; //相对原点的位置

    Bounds GetBounds()const; //返回焊盘的边界(矩形), 用于绘制使用, 比如焊盘需要不是正放置,那外接矩形就和焊盘不重叠

    bool HitTest(double x, double y, double tolerance) const override;//判断鼠标点击的点是否在焊盘上, 用于鼠标选择焊盘使用, 需要重写
    // 前两个参数判断鼠标的坐标, tolerance是容差, 也就是鼠标点击的点离焊盘的距离小于tolerance就认为是点击了焊盘


    bool Validate(std::string* error = nullptr) const override; //一个安全测试函数
    // 判断这些数据是否合法, 用于后续使用, 比如这个宽度高度不小于零这种
};


class GraphicItem : public FootprintItem //除了焊盘的其他元件, 圆弧,直线等
{
public:
    Layer layer = Layer::SilkScreen; //默认为丝印层
};



