// tests/test_DxfParser.cpp
#include <QtTest>
#include "core/DxfParser.h"

class TestDxfParser : public QObject {
    Q_OBJECT

private slots:
    void test_parse_aligned_dimension() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n0\n"
            "10\n100.0\n20\n200.0\n11\n120.0\n21\n180.0\n"
            "13\n0.0\n23\n0.0\n14\n200.0\n24\n100.0\n"
            "40\n50.0\n50\n45.0\n1\n50.0\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("aligned"));
        QCOMPARE(dims[0].value, 50.0);
        QCOMPARE(dims[0].centerX, 120.0);
        QCOMPARE(dims[0].centerY, 180.0);
        QCOMPARE(dims[0].rotation, 45.0);
    }

    void test_parse_linear_horizontal() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n1\n"
            "10\n0.0\n20\n0.0\n11\n100.0\n21\n-10.0\n"
            "40\n200.0\n50\n0.0\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("linear"));
        QCOMPARE(dims[0].orientation, std::string("horizontal"));
    }

    void test_parse_linear_vertical() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n1\n"
            "10\n0.0\n20\n0.0\n11\n110.0\n21\n50.0\n"
            "40\n100.0\n50\n90.0\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("linear"));
        QCOMPARE(dims[0].orientation, std::string("vertical"));
    }

    void test_parse_radial() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n4\n"
            "10\n50.0\n20\n50.0\n11\n80.0\n21\n30.0\n"
            "40\n30.0\n1\nR30\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("radial"));
        QCOMPARE(dims[0].value, 30.0);
    }

    void test_parse_diameter() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n3\n"
            "10\n50.0\n20\n50.0\n11\n90.0\n21\n50.0\n"
            "40\n40.0\n1\n40\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("diameter"));
        QCOMPARE(dims[0].value, 40.0);
    }

    void test_parse_angular() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n2\n"
            "10\n0.0\n20\n0.0\n11\n60.0\n21\n40.0\n"
            "40\n45.0\n1\n45°\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("angular"));
    }

    void test_no_dimensions_in_line_only_drawing() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0\n20\n0\n11\n100\n21\n100\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(0));
    }

    void test_handles_empty_content() {
        DxfParser parser;
        auto dims = parser.parseDimensions("");
        QCOMPARE(dims.size(), size_t(0));
    }

    void test_handles_windows_line_endings() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\r\nSECTION\r\n2\r\nENTITIES\r\n"
            "0\r\nDIMENSION\r\n8\r\n0\r\n70\r\n0\r\n"
            "10\r\n100.0\r\n20\r\n200.0\r\n11\r\n120.0\r\n21\r\n180.0\r\n"
            "40\r\n50.0\r\n50\r\n45.0\r\n"
            "0\r\nENDSEC\r\n0\r\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("aligned"));
        QCOMPARE(dims[0].value, 50.0);
    }

private:
    DxfParser parser;
};

QTEST_MAIN(TestDxfParser)
#include "test_DxfParser.moc"
