﻿/** MAXRF_Main: Main point of access to control the MAXRF instrument
 *  Copyright (C) 2020 Rodrigo Torres and contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ALL_TTY_H
#define ALL_TTY_H

#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
/* For synchronization with the DAQ */
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#include <fstream>
#include <sstream>
#include <cmath>

#include <QDebug>
#include <QLineEdit>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "MAXRF/ipc_methods.h"


static constexpr auto kStyleGreen { "QLineEdit {background-color: #2DC937; "
                                   "font-weight: bold; color: white;}" };
static constexpr auto kStyleYellow{ "QLineEdit {background-color: #E7B416; "
                                   "font-weight: bold; color: white;}" };

class tty_agent;

///
/// \brief The MotorCommands enum lists the available commands that can be sent
/// to the motor controller managed by  the \see StageMotor class.
///
enum class MotorCommands : int
{
  kIdentify,
  kInitSequence,
  kShutdownSequence,
  kGetPosition,
  kMoveToPosition,
  kStepIncrease,
  kStepDecrease,
  kGetSpeed,
  kSetSpeed,
  kGentleStop,
  kEmergencyHalt
};

///
/// \brief The TTYHandle class is a very wrapper around the POSIX terminal
/// interface functions (termios) for asynchronous communication ports. This
/// class is better suited to handle communication between a device and the
/// computer that is based on a command-reply structure
///
class TTYHandle {

public:
  ///
  /// \brief TTYHandle initializes a communication port with the hardcoded
  /// parameters
  ///
  /// The constructor of the class takes the path to a character device file,
  /// initializes it with some preconfigured parameter and throws in case the
  /// port cannot be opened.
  ///
  /// An object of this class that has been succesfully constructed is most likely
  /// in a valid state. In case of the underlying hardware device managed by the
  /// TTYHandle disconnects, the member functions \see SendData and \see
  /// ReadAvailableData throw an exception  informing  of the error
  ///
  /// \param port_path the filepath pointing to the character device file of the
  /// communication  port
  ///
  TTYHandle(std::string port_path);

  ///
  /// \brief ~TTYHandle closes the file descriptor refering to the communication
  /// port when an object of this class is destroyed
  ///
  virtual ~TTYHandle();

  ///
  /// \brief SendData
  ///
  void SendData(std::string);

  ///
  /// \brief ReadAvailableData
  /// \return
  ///
  std::string ReadAvailableData();

protected:
  int dev_fd { -1 };
  std::string df_path {};
};


enum class MonitorStyles : uint8_t {
  kRed    = 0x01,
  kYellow = 0x02,
  kGreen  = 0x04,
};

///
/// \brief The StageMotor class derives from the \see TTYHandle class and
/// provides additional functionality to control a stepper motor on a
/// communication port.
///
class StageMotor : protected TTYHandle {

public:
    StageMotor(std::string);
    ~StageMotor();



    QString TriggerPositionUpdate() {
      if (is_inited) {
        check_ont();
        SendCommand(MotorCommands::kGetPosition);
      }
      else {
        status_message_ = " Disconnected";
      }

      return QString::fromStdString(status_message_);
    }


    double SendCommand(MotorCommands command, double argument = -1.);

    QString get_message() {
      return QString::fromStdString(status_message_);
    }

    bool wait();
    void ConfigureStage();

    void check_ont();
    bool IsInited() const;
    bool IsOnTarget() const;

    double pos = 0;
private:
    std::map<MotorCommands, std::string> command_list_ {};
    std::string status_message_ {};
    std::string stage_name_ {};
    char stage_id_;


    bool on_target = false;
    bool is_inited = false;

    std::vector<std::string> params;
};

////
/// \brief The keyence class
///
class keyence : protected TTYHandle
{
public:
    keyence(std::string s) try : TTYHandle(s)
    {
      status_message_ = "Keyence: " + df_path;
    } catch(std::runtime_error & err) {
      qDebug() << err.what();
    }
    ~keyence() {}

    auto ReadDistanceOffset() {
        SendData("1");
        std::string diff = ReadAvailableData();
 //       offset = stod(diff);
        return std::stod(diff);
    }

    QString get_message() {
      return QString::fromStdString(status_message_);
    }
 private:
    std::string status_message_ {};
};


///
/// \brief The ScanManager class takes ownership of two stepper motor handles,
/// \see StageMotor, and begins a scan session when constructed
///
class ScanManager : public QObject {
    Q_OBJECT

public:

  using SHMStructure  = maxrf::ipc::SHMStructure;
  using MotorHandle   = std::unique_ptr<StageMotor>;

  ScanManager(MotorHandle x, MotorHandle y, tty_agent * handle);
  ~ScanManager() = default;

  void TakeBackOwnership(MotorHandle & x, MotorHandle & y);


  void UpdateEvent();
  void AbortScan();

  bool IsDone();

signals:
  void UpdatePositionMonitors(QString, QString, int);



private:
  bool scan_done_ { false };

  maxrf::daq::DAQModeParameters params {};
  maxrf::ipc::POSIXSemaphore sem_probe {};
  maxrf::ipc::POSIXSemaphore sem_reply {};

  std::unique_ptr<StageMotor> stage_x_ {nullptr};
  std::unique_ptr<StageMotor> stage_y_ {nullptr};
};

///
/// \brief The tty_agent class
///
class tty_agent : public QObject {
    Q_OBJECT

public:
  explicit tty_agent(QObject * parent = nullptr);

public slots:
    void relay_command(int, QString = "");

    void move_stage(int, double);
    void enable_servo(bool);
    void start_servo(bool);
    void abort();

    void DAQRequestRespond(bool accept) {
      daq_req_input_ready = true;
      daq_req_accepted    = accept;
    }

signals:
    void TriggerDAQPrompt();

    void toggle_tab1(bool);
    void update_statbar(QString);
    void update_monitor(QString, QString, int);
    void toggle_widgets(int);

private:
    std::atomic_bool daq_req_input_ready { false };
    std::atomic_bool daq_req_accepted    { false };


    std::unique_ptr<StageMotor> & HandleOfMotorID(int id);
    std::unique_ptr<ScanManager>  scan_handle_ { nullptr };

    void servo();
    void scan();
    void scan_loop();
    void timer_event();


    QString style_green  = "QLineEdit {background-color: #2DC937; font-weight: bold; color: white;}";
    QString style_yellow = "QLineEdit {background-color: #E7B416; font-weight: bold; color: white;}";
    sem_t * sem_reply;
    sem_t * sem_probe;


    bool scanning {false};      ///< Tracks if there's an ongoing scan
    bool laser_active {false};  ///< Tracks whether the laser is reading
    bool servo_active {false};  ///< Tracks whether the distance-correcting serve is active
    bool next_line = false;

    static constexpr int servo_interval {250}; ///< Update interval for laser tracking
    static constexpr double servo_threshold {200.0}; ///< Expressed in micrometers


    std::array<QLineEdit *, 4>     monitors_;

    std::unique_ptr<StageMotor> stage_x_ {nullptr};
    std::unique_ptr<StageMotor> stage_y_ {nullptr};
    std::unique_ptr<StageMotor> stage_z_ {nullptr};
    std::unique_ptr<keyence> laser_ {nullptr};

    QTimer *update_timer;
    QTimer *servo_timer;
};

#endif // ALL_TTY_H
