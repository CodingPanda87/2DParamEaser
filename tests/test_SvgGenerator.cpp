#include <QtTest>
#include "core/SvgGenerator.h"

class TestSvgGenerator : public QObject {
    Q_OBJECT

private slots:
    void test_generates_three_layers() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 200 200\">"
                              "<g id=\"drawing\"><line x1=\"0\" y1=\"0\" x2=\"100\" y2=\"100\"/></g></svg>";
        std::vector<Dimension> dims = {
            {.id = "D1", .type = "linear", .value = 100, .centerX = 50, .centerY = 20,
             .param = ParamBinding{"width", "float", 100}},
            {.id = "D2", .type = "radial", .value = 25, .centerX = 80, .centerY = 60,
             .param = std::nullopt},
        };
        std::string result = gen.generate(drawing, dims);
        QVERIFY(result.find("<g id=\"drawing\">") != std::string::npos);
        QVERIFY(result.find("<g id=\"dimensions\">") != std::string::npos);
        QVERIFY(result.find("<g id=\"params\">") != std::string::npos);
    }

    void test_has_bound_param_badge() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 100 100\"></svg>";
        std::vector<Dimension> dims = {
            {.id = "D1", .type = "linear", .value = 100, .centerX = 50, .centerY = 20,
             .param = ParamBinding{"width", "float", 100}},
        };
        std::string result = gen.generate(drawing, dims);
        QVERIFY(result.find("$width") != std::string::npos);
        QVERIFY(result.find("float") != std::string::npos);
    }

    void test_has_unbound_marker() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 100 100\"></svg>";
        std::vector<Dimension> dims = {
            {.id = "D2", .type = "linear", .value = 25, .centerX = 30, .centerY = 40,
             .param = std::nullopt},
        };
        std::string result = gen.generate(drawing, dims);
        QVERIFY(result.find("D2") != std::string::npos);
        QVERIFY(result.find("???") != std::string::npos);
        QVERIFY(result.find("param-badge-unbound") != std::string::npos);
    }

    void test_metadata_embedded() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 100 100\"></svg>";
        std::vector<Dimension> dims = {
            {.id = "D1", .type = "linear", .value = 50, .centerX = 10, .centerY = 20,
             .param = ParamBinding{"len", "int", 50}},
        };
        std::string result = gen.generateWithMetadata(drawing, dims, "test.dxf");
        QVERIFY(result.find("<metadata>") != std::string::npos);
        QVERIFY(result.find("test.dxf") != std::string::npos);
        QVERIFY(result.find("\"version\":\"1.0\"") != std::string::npos);
    }

    void test_int_badge_uses_int_class() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 100 100\"></svg>";
        std::vector<Dimension> dims = {
            {.id = "D1", .type = "linear", .value = 10, .centerX = 50, .centerY = 50,
             .param = ParamBinding{"count", "int", 10}},
        };
        std::string result = gen.generate(drawing, dims);
        QVERIFY(result.find("param-badge-int") != std::string::npos);
        QVERIFY(result.find("param-text-int") != std::string::npos);
    }

    void test_float_badge_uses_float_class() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 100 100\"></svg>";
        std::vector<Dimension> dims = {
            {.id = "D1", .type = "linear", .value = 10.5, .centerX = 50, .centerY = 50,
             .param = ParamBinding{"len", "float", 10.5}},
        };
        std::string result = gen.generate(drawing, dims);
        QVERIFY(result.find("param-badge-float") != std::string::npos);
        QVERIFY(result.find("param-text-float") != std::string::npos);
    }
};

QTEST_MAIN(TestSvgGenerator)
#include "test_SvgGenerator.moc"
