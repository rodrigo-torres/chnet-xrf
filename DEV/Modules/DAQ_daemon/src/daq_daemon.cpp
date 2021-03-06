/** MAXRF_DAQ: Manage XRF DAQ sessions with the CHNET built MAXRF Scanner
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

#include "daq_daemon.h"

namespace maxrf::daq {

class ScanDimensions
{
protected:
  friend class DAQSession;
  static uint width;
  static uint height;
  static uint pixels;

  void static ComputeDimensions(DAQModeParameters const & params) {
    auto val = params.x_end_coordinate - params.x_start_coordinate;
    val /= params.x_motor_step;

    width = static_cast<uint>(val);

    val = params.y_end_coordinate - params.y_start_coordinate;
    val /= params.y_motor_step;

    height = static_cast<uint>(val) + 1;
    pixels = width *  height;
  }

public:
  //  static inline auto PixelToXCoord(uint pixel) {
  //    return pixel % width;
  //  }
  //  static inline auto PixelToYCoord(uint pixel) {
  //    return pixel / width;
  //  }

  static inline std::pair<uint, uint> PixelToCoords(uint pixel) {
    bool const odd_line  = (pixel / width) % 2;

    if (odd_line) {
      return { (width - 1) - ( pixel % width), pixel / width };
    }
    else {
      return {pixel % width, pixel / width };
      //      return { width - (pixel % width), pixel / width };
    }
  }

  static inline auto Width() noexcept {
    return width;
  }

  static inline auto Height() noexcept {
    return height;
  }

  static inline auto Pixels() noexcept {
    return pixels;
  }

};

uint ScanDimensions::width  {0};
uint ScanDimensions::height {0};
uint ScanDimensions::pixels {0};

bool DAQSession::SetupDAQSession(DAQInitParameters const & config) {
  bool status {false};

  if (config.mode_parameters.mode == DAQMode::kDAQInvalid) {
    return status;
  }

  try {
    ScanDimensions::ComputeDimensions(config.mode_parameters);
    caen_handle_ = std::make_unique<CAENLibraryHandle>();

    auto writer_ = std::make_shared<FileWriter>();
    writer_->Initialize(config);

    uint pipes_id {0};
    for (auto & board : config.boards_config) {
      //      auto ptr = caen_handle_->GetBoardHandle(board);
      pipes_.push_back(DAQPipe {caen_handle_->GetBoardHandle(board), writer_});
      pipes_.back().SetID(pipes_id);
      ++pipes_id;
    }
    status = true;
  } catch (std::exception & e) {
    std::cout << e.what() << std::endl;
  }
  //  daq_duration = std::chrono::duration<double>(config.mode_parameters.timeout);
  session_params_ = config.mode_parameters;

  return status;
}

void DAQSession::StartDAQSession() {

  switch (session_params_.mode) {
  case DAQMode::kDAQScan :
#ifndef DAQ_DAEMON_DEBUG
    std::cout << "Initing semaphores... " << std::endl;
    sem_probe_.Init("/daq_probe");
    sem_reply_.Init("/daq_reply");
    std::cout << "Semaphores inited... " << std::endl;
    sem_reply_.Post();
    sem_probe_.Wait();
#endif
    [[fallthrough]];
  case DAQMode::kDAQPoint :
    daq_enable.store(true);
    EnterDAQLoop();
    break;
  case DAQMode::kDAQInvalid :
    break;
  }
}

void DAQSession::EnterDAQLoop() {
  using namespace std::chrono;

  uint32_t pixel {0};
  auto daq_duration   = std::chrono::duration<double> {session_params_.timeout};


  int32_t samples {0};
  microseconds micros {0};

  pipes_.front().OpenPipe();
  auto start_timestamp = steady_clock::now();
  while (daq_enable) {
    if (session_params_.mode == DAQMode::kDAQScan) {
      // Time period is less than the dwell time per pixel
      if (steady_clock::now() - start_timestamp < daq_duration) {
        auto t1 = steady_clock::now();
        pipes_.front().CollectData();
        auto t2 = steady_clock::now();
        micros += duration_cast<microseconds>(t2 - t1);
        ++samples;
        std::this_thread::sleep_for(daq_duration / 10);
        continue;
      }
      micros /= samples;
      std::cout << "Collecting data took (us): " << micros.count() << std::endl;
      micros = microseconds {0};
      samples = 0;
      //    auto t1 = steady_clock::now();
      pipes_.front().SendDataDownPipe();
      //    auto t2 = steady_clock::now();
      //    std::cout << "Exchange took (us): "
      //              <<  duration_cast<microseconds>(t2  - t1).count() << std::endl;
      ++pixel;

      if (pixel == ScanDimensions::Pixels()) {
        pipes_.front().ClosePipe();
#ifndef DAQ_DAEMON_DEBUG
        sem_reply_.Post();  // One last post to let know app we've finished
#endif
        break;
      }

      if ( (pixel % ScanDimensions::Width()) == 0) {
        // Synchronize with the motors at the end of each scan line
        pipes_.front().ClosePipe();
#ifndef DAQ_DAEMON_DEBUG
        sem_reply_.Post();
        sem_probe_.Wait();
#endif
        pipes_.front().OpenPipe();
      }
      start_timestamp = steady_clock::now();
    }
    else if (session_params_.mode == DAQMode::kDAQPoint) {
      if (steady_clock::now() - start_timestamp > daq_duration) {
        break;
      }
      std::this_thread::sleep_for(500ms);
      pipes_.front().CollectData();
      pipes_.front().SendDataDownPipe();
    }
    else {
      break;
    }
  }

  CleanupEverything();
}

void DAQSession::CleanupEverything() {
  session_params_.mode = DAQMode::kDAQInvalid;
  pipes_.clear();

  caen_handle_.reset(nullptr);
}

void DAQPipe::SendDataDownPipe() {
  if (!pipe_open_) {
    return;
  }

  uint32_t channel_id {0};
  auto coords = ScanDimensions::PixelToCoords(pixel);
  for (auto & channel : producer_->GetHandles()) {
    auto packet = consumer_->GetEmptyDataPacket();
    packet.channel_id = pipe_id + channel_id;
    packet.pixel.coord_x = coords.first;
    packet.pixel.coord_y = coords.second;
    packet.pixel.histogram.swap(channel.histogram);
    ++pixel;

    consumer_->PushDataPacketToQueue(std::move(packet));
    ++channel_id;
  }
}

FileWriter::FileWriter() {}



void FileWriter::Initialize(DAQInitParameters const & config) {
  using namespace std::filesystem;
  std::error_code err;

  path file_path {config.output_path};
  if (!exists(file_path, err)) {
    // We attempt to create the directory
    if (!create_directory(file_path, err)) {
      throw std::runtime_error(err.message());
    }
  }
  file_path /= config.base_filename;

  std::stringstream filename;
  for (auto & board : config.boards_config) {
    for (auto & channel : board.channels) {
      // Prepare the filename based  upon the base name
      filename.str("");
      filename << config.base_filename << '_'
               << "b" << std::to_string(board.board_id) << '_'
               << "c" << std::to_string(channel.first) << ".hyperc";
      file_path.replace_filename(filename.str());

      // Ask for a pointer to a File Handle
      auto ptr = maxrf::DataFileHander::GetFileForDAQ(file_path.c_str());
      if (ptr->GetFormat() != DataFormat::kHypercube) {
        throw std::runtime_error("Can't");
      }
      files_.push_back(std::static_pointer_cast<maxrf::HypercubeFile>(ptr));
      auto & file = files_.back();
      file->MakeDefaultHeader();

      // Fill-in the missing header entries
      file->EditToken("Name", "Test");
      file->EditToken("Description", "DAQ tests with new CAENDPPLib");

      file->EditToken("Calibration_Param_0", "0.00");
      file->EditToken("Calibration_Param_1", "0.0113");
      file->EditToken("Calibration_Param_2", "0.00");
      file->EditToken("Width", std::to_string(ScanDimensions::Width()));
      file->EditToken("Height", std::to_string(ScanDimensions::Height()));
      file->EditToken("Pixels", std::to_string(ScanDimensions::Pixels()));
    }
  }

  mode_ = static_cast<WriteMode>(static_cast<int>(config.mode_parameters.mode));
  // file->SetWriteMode(mode, ScanDimensions::Width());


  constexpr int kEmptyBuffersSize = 10;
  for (auto _ = kEmptyBuffersSize; _--; ) {
    DataPacket packet {};
    packet.pixel.histogram.resize(kSpectralBins, 0);
    empty_packets_pool.push(std::move(packet));
  }

  TimestampFiles();
  BeginWritingTask();
}

void FileWriter::TimestampFiles() {
  std::stringstream sstream;

  using std::chrono::system_clock;
  auto t_c = system_clock::to_time_t(system_clock::now());
  sstream << std::put_time(std::localtime(&t_c), "%Y-%m-%d %H-%M-%S");

  for (auto & file : files_) {
    file->EditToken(HeaderTokens::kDAQStartTimestamp, sstream.str());
    file->WriteHeader();
    file->SetWriteMode(mode_, ScanDimensions::Width());
  }
}

void FileWriter::WriteTask() {
  Job current_job;

  while (daq_enable || !pending_jobs.empty()) {
    if (pending_jobs.try_pop(current_job)) {
      //      using namespace std::chrono;
      //      auto t1 = high_resolution_clock::now();
      //      WriteDataToFiles(current_job);

      files_.at(current_job.channel_id)->WritePixel(current_job.pixel);
      // We have already zeroed the packet buffer
      current_job.pixel.coord_x   = std::numeric_limits<int32_t>::max();
      current_job.pixel.coord_y   = std::numeric_limits<int32_t>::max();
      current_job.channel_id = std::numeric_limits<int32_t>::max();

      empty_packets_pool.push(std::move(current_job));
      //      auto t2 = high_resolution_clock::now();
      //      std::cout << "Writing to file took (ms): "
      //                << duration_cast<microseconds>(t2 - t1).count() << std::endl;
      //      For writing spectra is on the order of  a few tenths of microseconds
      // Except! for when the tasks flushes out the line buffer
      // In which case it seems to take ~40-100 microseconds per pixel
    }
  }

  for (auto & file_handle : files_) {
    file_handle->WriteFooter();
  }
}


} //namespace maxrf::daq
