// Scenario 1 of 3: GUI events, measured on a real framework (Qt 5.15).
//
// The point of this file: ONE signal, and a CONNECTION TYPE argument decides which
// rung of the ladder you land on.
//
//   DirectConnection -> the slot runs inside emit()            -> rung 3 (synchronous bus)
//   QueuedConnection -> emit only posts; the event loop runs it -> rung 4 (queue + dispatcher)
//   AutoConnection   -> the framework picks, from thread affinity
//
// Rung position is not a property of this design. It is an argument to connect().
//
// Build/run: ./run_scenarios.sh

#include <QCoreApplication>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QThread>

#include <cstdio>

static void Say(const QString& text) {
  std::fprintf(stdout, "%s\n", text.toUtf8().constData());
}

class Sensor : public QObject {
  Q_OBJECT

 public:
  void Tick(double value) {
    Say(QStringLiteral("-- publish %1").arg(value, 0, 'f', 1));
    emit Reading(value);
    Say(QStringLiteral("-- publish %1 returned").arg(value, 0, 'f', 1));
  }

 signals:
  void Reading(double value);
};

class Display : public QObject {
  Q_OBJECT

 public:
  explicit Display(const QString& name) : name_(name) {}

  // Public so main() can read the count on whichever thread owns this object.
  int hits() const { return hits_; }

 public slots:
  void OnReading(double value) {
    ++hits_;
    Say(QStringLiteral("   [%1] got %2 (hits=%3)")
            .arg(name_)
            .arg(value, 0, 'f', 1)
            .arg(hits_));
  }

 private:
  QString name_;
  int hits_ = 0;
};

int main(int argc, char** argv) {
  setvbuf(stdout, nullptr, _IOLBF, 0);
  QCoreApplication app(argc, argv);

  {
    Say(QStringLiteral("=== A. DirectConnection ==="));
    Sensor sensor;
    Display display(QStringLiteral("direct"));
    QObject::connect(&sensor, &Sensor::Reading, &display, &Display::OnReading,
                     Qt::DirectConnection);
    sensor.Tick(25.0);
    Say(QStringLiteral("   after Tick returned: hits=%1  <- already delivered")
            .arg(display.hits()));
  }

  {
    Say(QStringLiteral(""));
    Say(QStringLiteral("=== B. QueuedConnection ==="));
    Sensor sensor;
    Display display(QStringLiteral("queued"));
    QObject::connect(&sensor, &Sensor::Reading, &display, &Display::OnReading,
                     Qt::QueuedConnection);
    sensor.Tick(25.0);
    Say(QStringLiteral("   after Tick returned: hits=%1  <- nothing delivered yet")
            .arg(display.hits()));
    Say(QStringLiteral("   -- running the event loop --"));
    QCoreApplication::processEvents();
    Say(QStringLiteral("   after event loop: hits=%1").arg(display.hits()));
  }

  {
    Say(QStringLiteral(""));
    Say(QStringLiteral("=== C. AutoConnection across threads ==="));
    QThread worker;
    worker.start();
    Sensor sensor;
    Display display(QStringLiteral("auto/cross-thread"));
    display.moveToThread(&worker);
    QObject::connect(&sensor, &Sensor::Reading, &display, &Display::OnReading,
                     Qt::AutoConnection);
    sensor.Tick(25.0);
    Say(QStringLiteral("   after Tick returned: hits=0 by definition "
                       "(hits() is on the other thread)"));

    // Ask the owning thread for the count, once its event loop has drained.
    int hits = 0;
    QMetaObject::invokeMethod(
        &display, [&display, &hits] { hits = display.hits(); },
        Qt::BlockingQueuedConnection);
    Say(QStringLiteral("   after the worker drained: hits=%1  <- Auto chose Queued")
            .arg(hits));
    worker.quit();
    worker.wait();
  }

  return 0;
}

#include "qt_connection_types.moc"
