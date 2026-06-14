// tests/test_ProjectManager.cpp
#include <QtTest>
#include <memory>
#include "core/ProjectManager.h"

class TestProjectManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        pm = std::make_unique<ProjectManager>();
        DrawingData dd;
        dd.key = "drawing1";
        dd.sourceFile = "test.dxf";
        dd.rawDxfContent = "";
        dd.drawingSvg = "";
        dd.dimensions = {
            Dimension{.id = "D1", .type = "aligned", .value = 50.0, .centerX = 10, .centerY = 20},
            Dimension{.id = "D2", .type = "linear", .value = 100.0, .centerX = 30, .centerY = 40},
        };
        pm->addDrawing("drawing1", dd);
    }

    void test_add_and_get_drawing() {
        QCOMPARE(pm->drawingCount(), size_t(1));
        auto* d = pm->getDrawing("drawing1");
        QVERIFY(d != nullptr);
        QCOMPARE(d->dimensions.size(), size_t(2));
        QVERIFY(pm->hasDrawing("drawing1"));
        QVERIFY(!pm->hasDrawing("nonexistent"));
    }

    void test_add_duplicate_rejected() {
        DrawingData dd;
        dd.key = "drawing1";
        bool ok = pm->addDrawing("drawing1", dd);
        QVERIFY(!ok); // duplicate
    }

    void test_bind_param() {
        bool ok = pm->bindParam("drawing1", "D1", "float", "hole_dia", 50.0);
        QVERIFY(ok);
        auto* dim = pm->findDimension("drawing1", "D1");
        QVERIFY(dim->isBound());
        QCOMPARE(dim->param->key, std::string("hole_dia"));
        QCOMPARE(dim->param->type, std::string("float"));
        QCOMPARE(dim->param->defaultValue, 50.0);
    }

    void test_unbind_param() {
        pm->bindParam("drawing1", "D1", "float", "hole_dia", 50.0);
        pm->unbindParam("drawing1", "D1");
        auto* dim = pm->findDimension("drawing1", "D1");
        QVERIFY(!dim->isBound());
    }

    void test_statistics() {
        pm->bindParam("drawing1", "D1", "float", "a", 1);
        QCOMPARE(pm->boundCount("drawing1"), 1);
        QCOMPARE(pm->unboundCount("drawing1"), 1);
        QCOMPARE(pm->totalCount("drawing1"), 2);
    }

    void test_remove_drawing() {
        DrawingData dd2;
        dd2.key = "temp";
        pm->addDrawing("temp", dd2);
        QCOMPARE(pm->drawingCount(), size_t(2));
        pm->removeDrawing("temp");
        QCOMPARE(pm->drawingCount(), size_t(1));
    }

    void test_get_markers() {
        pm->bindParam("drawing1", "D1", "float", "length", 50.0);
        auto markers = pm->getMarkers("drawing1");
        QCOMPARE(markers.size(), size_t(2));

        auto m1 = std::find_if(markers.begin(), markers.end(),
            [](const DimensionMarker& m) { return m.id == "D1"; });
        QVERIFY(m1 != markers.end());
        QVERIFY(m1->isBound);
        QCOMPARE(m1->label, std::string("length"));

        auto m2 = std::find_if(markers.begin(), markers.end(),
            [](const DimensionMarker& m) { return m.id == "D2"; });
        QVERIFY(m2 != markers.end());
        QVERIFY(!m2->isBound);
        QCOMPARE(m2->label, std::string("D2"));
    }

    void test_to_json() {
        pm->bindParam("drawing1", "D1", "float", "length", 50.0);
        std::string json = pm->toJson();
        QVERIFY(json.find("\"version\":\"1.0\"") != std::string::npos);
        QVERIFY(json.find("drawing1") != std::string::npos);
        QVERIFY(json.find("length") != std::string::npos);
        QVERIFY(json.find("float") != std::string::npos);
    }

    void test_multiple_drawings_independent() {
        DrawingData dd2;
        dd2.key = "drawing2";
        dd2.dimensions = {
            Dimension{.id = "D1", .type = "radial", .value = 25.0, .centerX = 5, .centerY = 5},
        };
        pm->addDrawing("drawing2", dd2);
        pm->bindParam("drawing2", "D1", "int", "count", 10);

        // drawing1 should not be affected
        auto markers1 = pm->getMarkers("drawing1");
        QVERIFY(markers1.size() > 0);
        QCOMPARE(pm->totalCount("drawing2"), 1);
        QCOMPARE(pm->boundCount("drawing2"), 1);
        QCOMPARE(pm->unboundCount("drawing2"), 0);
    }

private:
    std::unique_ptr<ProjectManager> pm;
};

QTEST_MAIN(TestProjectManager)
#include "test_ProjectManager.moc"
