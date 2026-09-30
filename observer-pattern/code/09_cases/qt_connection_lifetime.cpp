// Case 1 of 4: Qt 5.15 signals/slots, measured on the real framework.
//
// Signal/slot is a synchronous push with the connection list owned by the SENDER
// (see 08-边界.md for the rung scale). This file does not re-measure the rung.
// It asks the question that section 06-生命周期负债.md left open: when the list
// is being walked, what may change underneath it?
//
//   A. a receiver dies                -> what happens to its entry
//   B. a slot disconnects a sibling   -> does the sibling still get called
//   C. the handle connect() returned  -> can it be used to cancel, and what does it say afterwards
//   D. the same slot connected twice  -> is the list a set or a sequence
//
// Every answer below is printed by the framework itself, not asserted here.
//
// Build/run: ./run_cases.sh

#include <QCoreApplication>
#include <QMetaObject>
#include <QObject>
#include <QString>

#include <cstdio>
#include <cstdlib>
#include <new>

// ---- replacement global allocator: used in section E ----------------------
static long g_alloc = 0;
static long g_free = 0;

void* operator new(std::size_t n) {
  ++g_alloc;
  void* p = std::malloc(n ? n : 1);
  if (p == nullptr) throw std::bad_alloc();
  return p;
}

void* operator new[](std::size_t n) {
  return ::operator new(n);
}

void operator delete(void* p) noexcept {
  if (p != nullptr) ++g_free;
  std::free(p);
}

void operator delete[](void* p) noexcept {
  ::operator delete(p);
}

void operator delete(void* p, std::size_t) noexcept {
  ::operator delete(p);
}

void operator delete[](void* p, std::size_t) noexcept {
  ::operator delete(p);
}

static void Say(const QString& text) {
  std::fprintf(stdout, "%s\n", text.toUtf8().constData());
}

// QObject::receivers() is protected in Qt. Deriving is the only way to read the
// count from outside the class; case A below prints it before and after a delete.
class Sensor : public QObject {
  Q_OBJECT

 public:
  void Tick(double value) {
    Say(QStringLiteral("-- emit %1").arg(value, 0, 'f', 1));
    emit Reading(value);
    Say(QStringLiteral("-- emit %1 returned").arg(value, 0, 'f', 1));
  }

  int ReceiverCount() const {
    return receivers(SIGNAL(Reading(double)));
  }

 signals:
  void Reading(double value);
};

class Display : public QObject {
  Q_OBJECT

 public:
  explicit Display(const QString& name) : name_(name) {}
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
    Say(QStringLiteral("=== A. the receiver is destroyed ==="));
    Sensor sensor;
    Display* display = new Display(QStringLiteral("heap"));
    QObject::connect(&sensor, &Sensor::Reading, display, &Display::OnReading);
    Say(QStringLiteral("   receivers() before emit: %1").arg(sensor.ReceiverCount()));
    sensor.Tick(1.0);
    delete display;
    Say(QStringLiteral("   receivers() after  delete: %1  <- nobody called disconnect()")
            .arg(sensor.ReceiverCount()));
    sensor.Tick(2.0);
    Say(QStringLiteral("   <- the second emit produced no slot output"));
  }

  {
    Say(QStringLiteral(""));
    Say(QStringLiteral("=== B1. a slot disconnects a sibling, mid-walk ==="));
    Sensor sensor;
    Display first(QStringLiteral("first"));
    Display second(QStringLiteral("second"));
    Display third(QStringLiteral("third"));
    QMetaObject::Connection c_second;
    QMetaObject::Connection c_third;
    int first_calls = 0;
    QObject::connect(&sensor, &Sensor::Reading, &first,
                     [&c_second, &first_calls](double) {
                       ++first_calls;
                       Say(QStringLiteral("   [first] disconnecting [second]"));
                       QObject::disconnect(c_second);
                     });
    c_second = QObject::connect(&sensor, &Sensor::Reading, &second, &Display::OnReading);
    c_third = QObject::connect(&sensor, &Sensor::Reading, &third, &Display::OnReading);
    Say(QStringLiteral("   receivers() = %1").arg(sensor.ReceiverCount()));
    sensor.Tick(1.0);
    Say(QStringLiteral("   after the round: first=%1 second=%2 third=%3")
            .arg(first_calls)
            .arg(second.hits())
            .arg(third.hits()));
  }

  {
    Say(QStringLiteral(""));
    Say(QStringLiteral("=== B2. a slot disconnects itself, mid-walk ==="));
    Sensor sensor;
    Display other(QStringLiteral("other"));
    int self_hits = 0;
    QMetaObject::Connection c_self;
    c_self = QObject::connect(&sensor, &Sensor::Reading, &sensor,
                              [&c_self, &self_hits](double) {
                                ++self_hits;
                                Say(QStringLiteral("   [self] disconnecting itself"));
                                QObject::disconnect(c_self);
                              });
    QObject::connect(&sensor, &Sensor::Reading, &other, &Display::OnReading);
    sensor.Tick(1.0);
    Say(QStringLiteral("   after round 1: self=%1 other=%2").arg(self_hits).arg(other.hits()));
    sensor.Tick(2.0);
    Say(QStringLiteral("   after round 2: self=%1 other=%2").arg(self_hits).arg(other.hits()));
  }

  {
    Say(QStringLiteral(""));
    Say(QStringLiteral("=== B3. a slot deletes a sibling, mid-walk ==="));
    Sensor sensor;
    Display first(QStringLiteral("first"));
    Display* third = new Display(QStringLiteral("third"));
    Display fourth(QStringLiteral("fourth"));
    int first_calls = 0;
    QObject::connect(&sensor, &Sensor::Reading, &first,
                     [&first_calls, third](double) {
                       ++first_calls;
                       Say(QStringLiteral("   [first] deleting [third]"));
                       delete third;
                     });
    QObject::connect(&sensor, &Sensor::Reading, third, &Display::OnReading);
    QObject::connect(&sensor, &Sensor::Reading, &fourth, &Display::OnReading);
    sensor.Tick(1.0);
    Say(QStringLiteral("   after the round: first=%1 fourth=%2  <- survived the delete")
            .arg(first_calls)
            .arg(fourth.hits()));
  }

  {
    Say(QStringLiteral(""));
    Say(QStringLiteral("=== C. the handle connect() returned ==="));
    Sensor sensor;
    Display display(QStringLiteral("handle"));
    QMetaObject::Connection c =
        QObject::connect(&sensor, &Sensor::Reading, &display, &Display::OnReading);
    Say(QStringLiteral("   bool(handle) = %1").arg(c ? QStringLiteral("true")
                                                     : QStringLiteral("false")));
    sensor.Tick(1.0);
    QObject::disconnect(c);
    Say(QStringLiteral("   after QObject::disconnect(handle): bool = %1   receivers() = %2")
            .arg(c ? QStringLiteral("true") : QStringLiteral("false"))
            .arg(sensor.ReceiverCount()));
    sensor.Tick(2.0);
    Say(QStringLiteral("   hits = %1  <- the handle outlived the connection, and says so")
            .arg(display.hits()));
  }

  {
    Say(QStringLiteral(""));
    Say(QStringLiteral("=== D. the same slot connected twice ==="));
    Sensor sensor;
    Display display(QStringLiteral("dup"));
    QObject::connect(&sensor, &Sensor::Reading, &display, &Display::OnReading);
    QObject::connect(&sensor, &Sensor::Reading, &display, &Display::OnReading);
    Say(QStringLiteral("   receivers() = %1  <- the list is a sequence, not a set")
            .arg(sensor.ReceiverCount()));
    sensor.Tick(1.0);

    Say(QStringLiteral(""));
    Sensor sensor2;
    QObject::connect(&sensor2, &Sensor::Reading, &display, &Display::OnReading,
                     Qt::UniqueConnection);
    QObject::connect(&sensor2, &Sensor::Reading, &display, &Display::OnReading,
                     Qt::UniqueConnection);
    Say(QStringLiteral("   with Qt::UniqueConnection: receivers() = %1")
            .arg(sensor2.ReceiverCount()));
    sensor2.Tick(2.0);
    Say(QStringLiteral("   cumulative hits = %1").arg(display.hits()));
  }

  {
    Say(QStringLiteral(""));
    Say(QStringLiteral("=== E. the price of one connection ==="));
    Sensor sensor;
    Display display(QStringLiteral("E"));
    long a0 = g_alloc;
    QMetaObject::Connection cold =
        QObject::connect(&sensor, &Sensor::Reading, &display, &Display::OnReading);
    long a1 = g_alloc;
    QMetaObject::Connection warm =
        QObject::connect(&sensor, &Sensor::Reading, &display, &Display::OnReading);
    long a2 = g_alloc;
    Say(QStringLiteral("   connect() #1 (cold): heap allocations = %1").arg(a1 - a0));
    Say(QStringLiteral("   connect() #2 (warm): heap allocations = %1").arg(a2 - a1));
    long f0 = g_free;
    QObject::disconnect(warm);
    Say(QStringLiteral("   disconnect():        heap frees = %1   allocations = %2")
            .arg(g_free - f0)
            .arg(g_alloc - a2));
    Say(QStringLiteral("   totals so far: allocated=%1 freed=%2").arg(g_alloc).arg(g_free));
    QObject::disconnect(cold);
  }

  return 0;
}

#include "qt_connection_lifetime.moc"
