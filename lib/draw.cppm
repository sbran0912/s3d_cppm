module;

#include <vector>

export module draw;

import color;
import math_core;
import mesh;
import renderer;

// ====================================================================
// ZEICHEN-FASSADE
// ====================================================================

export struct PrimitiveStyle {
    ColorInput stroke;
    ColorInput fill;
    double lineWidth = 0;
    double pointSize = 0;
};

export struct ShapeStyle {
    ColorInput stroke;
    ColorInput fill;
    double lineWidth = 0;
    double pointSize = 0;
    DrawStyle mode = StyleStroke;
    double z = 0;
    int segments = 0;
};

export void DrawMesh(const Mesh& mesh, const ShapeStyle& style,
                     const Matrix4x4& model = IdentityMatrix());

export void DrawMeshInstanced(const Mesh& mesh, const ShapeStyle& style,
                              const std::vector<Instance>& instances);

export void DrawPoint(double x, double y, double z, const PrimitiveStyle& style,
                      const Matrix4x4& model = IdentityMatrix());

export void DrawLine(double x1, double y1, double z1,
                     double x2, double y2, double z2,
                     const PrimitiveStyle& style,
                     const Matrix4x4& model = IdentityMatrix());

export void DrawTriangle(double x1, double y1, double z1,
                         double x2, double y2, double z2,
                         double x3, double y3, double z3,
                         const ShapeStyle& style,
                         const Matrix4x4& model = IdentityMatrix());

export void DrawShape(double x1, double y1, double z1,
                      double x2, double y2, double z2,
                      double x3, double y3, double z3,
                      double x4, double y4, double z4,
                      const ShapeStyle& style,
                      const Matrix4x4& model = IdentityMatrix());

export void DrawRect(double x, double y, double w, double h,
                     const ShapeStyle& style,
                     const Matrix4x4& model = IdentityMatrix());

export void DrawCircle(double x, double y, double z, double radius,
                       const ShapeStyle& style,
                       const Matrix4x4& model = IdentityMatrix());

export void DrawPolygon(const std::vector<double>& pts,
                        const ShapeStyle& style,
                        const Matrix4x4& model = IdentityMatrix());

namespace {

void ApplyStyle(const ColorInput& stroke, const ColorInput& fill,
                double lineWidth, double pointSize) {
    if (stroke.Set) StrokeColor(stroke.Color);
    if (fill.Set) FillColor(fill.Color);
    if (lineWidth != 0) StrokeWidth(lineWidth);
    if (pointSize != 0) PointSize(pointSize);
}

void ApplyStyle(const PrimitiveStyle& o) {
    ApplyStyle(o.stroke, o.fill, o.lineWidth, o.pointSize);
}

void ApplyStyle(const ShapeStyle& o) {
    ApplyStyle(o.stroke, o.fill, o.lineWidth, o.pointSize);
}

} // namespace

export void DrawMesh(const Mesh& mesh, const ShapeStyle& style, const Matrix4x4& model) {
    ApplyStyle(style);
    if (style.mode == StyleFill || style.mode == StyleBoth) {
        SubmitTriangles(mesh.Triangles, model);
    }
    if (style.mode == StyleStroke || style.mode == StyleBoth) {
        SubmitLines(mesh.Lines, model);
    }
}

export void DrawMeshInstanced(const Mesh& mesh, const ShapeStyle& style,
                       const std::vector<Instance>& instances) {
    ApplyStyle(style);
    SubmitInstanced(mesh.Triangles, mesh.Lines, style.mode, instances);
}

export void DrawPoint(double x, double y, double z, const PrimitiveStyle& style,
               const Matrix4x4& model) {
    ApplyStyle(style);
    Point(x, y, z, model);
}

export void DrawLine(double x1, double y1, double z1,
              double x2, double y2, double z2,
              const PrimitiveStyle& style, const Matrix4x4& model) {
    ApplyStyle(style);
    Line(x1, y1, z1, x2, y2, z2, model);
}

export void DrawTriangle(double x1, double y1, double z1,
                  double x2, double y2, double z2,
                  double x3, double y3, double z3,
                  const ShapeStyle& style, const Matrix4x4& model) {
    ApplyStyle(style);
    Triangle(x1, y1, z1, x2, y2, z2, x3, y3, z3, style.mode, model);
}

export void DrawShape(double x1, double y1, double z1,
               double x2, double y2, double z2,
               double x3, double y3, double z3,
               double x4, double y4, double z4,
               const ShapeStyle& style, const Matrix4x4& model) {
    ApplyStyle(style);
    Shape(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, style.mode, model);
}

export void DrawRect(double x, double y, double w, double h,
              const ShapeStyle& style, const Matrix4x4& model) {
    ApplyStyle(style);
    Rect(x, y, w, h, style.mode, style.z, model);
}

export void DrawCircle(double x, double y, double z, double radius,
                const ShapeStyle& style, const Matrix4x4& model) {
    ApplyStyle(style);
    int segments = style.segments;
    if (segments == 0) segments = 64;
    Circle(x, y, z, radius, style.mode, segments, model);
}

export void DrawPolygon(const std::vector<double>& pts,
                 const ShapeStyle& style, const Matrix4x4& model) {
    ApplyStyle(style);
    Polygon(pts, style.mode, model);
}
