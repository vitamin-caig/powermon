#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <numeric>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace {
  enum Sensor {
    VOLTAGE = 0,
    CURRENT = 1,
    POWER = 2,
    ENERGY = 3,
    TEST = 4
  };
  
  static const std::array<uint8_t, 4> DEFAULT_ADDR = {{0xc0, 0xa8, 0x01, 0x01}};
  
  void Ensure(bool cond, const std::string& msg) {
    if (!cond) {
      throw std::runtime_error(msg);
    }
  }

  struct Packet {
    std::array<uint8_t, 7> Body = {0};

  protected:
    uint8_t GetChecksum() const {
      return static_cast<uint8_t>(std::accumulate(Body.begin(), Body.begin() + 6, unsigned(0)));
    }
  };

  struct Request : Packet {
    explicit Request(Sensor sens) {
      Body[0] = 0xb0 | sens;
      std::copy(DEFAULT_ADDR.begin(), DEFAULT_ADDR.end(), Body.begin() + 1);
      Body[6] = GetChecksum();
    }
  };
  
  struct Response : Packet {
    float Get(Sensor sens) const {
      Ensure(Body[0] == (0xa0 | sens), "Invalid response type");
      Ensure(Body[6] == GetChecksum(), "Invalid checksum");
      switch (sens) {
      case VOLTAGE:
        Ensure(Body[4] == 0, "VOLTAGE: Body[4]");
        Ensure(Body[5] == 0, "VOLTAGE: Body[5]");
        return (256 * Body[1] + Body[2]) + float(Body[3]) / 10;
      case CURRENT:
        Ensure(Body[4] == 0, "CURRENT: Body[4]");
        Ensure(Body[5] == 0, "CURRENT: Body[5]");
        return (256 * Body[1] + Body[2]) + float(Body[3]) / 100;
      case POWER:
        Ensure(Body[3] == 0, "POWER: Body[3]");
        Ensure(Body[4] == 0, "POWER: Body[4]");
        Ensure(Body[5] == 0, "POWER: Body[5]");
        return (256 * Body[1] + Body[2]);
      case ENERGY:
        Ensure(Body[4] == 0, "ENERGY: Body[4]");
        Ensure(Body[5] == 0, "ENERGY: Body[5]");
        return 65536 * Body[1] + 256 * Body[2] + Body[3];
      case TEST:
        Ensure(Body[1] == 0, "TEST: Body[1]");
        Ensure(Body[2] == 0, "TEST: Body[2]");
        Ensure(Body[3] == 0, "TEST: Body[3]");
        Ensure(Body[4] == 0, "TEST: Body[4]");
        Ensure(Body[5] == 0, "TEST: Body[5]");
        return 0;
      }
    }
  };
  
  class Device {
  private:
    const int Handle;

  public:
    explicit Device(const std::string& path)
      : Handle( ::open(path.c_str(), O_RDWR | O_NOCTTY | O_SYNC))
    {
      CheckResult(Handle != -1, "open");
      ::termios ntio;
      CheckResult( ::tcgetattr(Handle, &ntio) == 0, "get properties");
      ntio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
      ntio.c_oflag &= ~(OPOST);
      ntio.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

      ntio.c_cflag &= ~(CSIZE | PARENB);
      ntio.c_cflag |= CS8;

      ntio.c_cc[VMIN] = sizeof(Response);
      ntio.c_cc[VTIME] = 1;

      const int SPEED = B9600;
      CheckResult( ::cfsetispeed(&ntio, SPEED) >= 0, "set ispeed");
      CheckResult( ::cfsetospeed(&ntio, SPEED) >= 0, "set ospeed");

      CheckResult( ::tcsetattr(Handle, TCSANOW, &ntio) == 0, "set properties");
    }
    
    ~Device() {
      ::close(Handle);
    }
    
    float Get(Sensor sens) {
      SendRequest(sens);
      WaitResponse();
      return ReadResponse(sens);
    }
  private:
    void CheckResult(bool res, const char* op) {
      Ensure(res, std::string("Failed to ") + op + ": " + std::strerror(errno));
    }
    
    void SendRequest(Sensor sens) {
      const Request req(sens);
      CheckResult( ::write(Handle, &req, sizeof(req)) == sizeof(req), "send request");
    }
    
    void WaitResponse() {
      struct timeval timeout = {1, 0};
      fd_set readSet;
      FD_ZERO(&readSet);
      FD_SET(Handle, &readSet);
      Ensure( ::select(Handle + 1, &readSet, 0, 0, &timeout) > 0 && FD_ISSET(Handle, &readSet), "Timeout while reading");
    }
    
    float ReadResponse(Sensor sens) {
      Response resp;
      CheckResult( ::read(Handle, &resp, sizeof(resp)) == sizeof(resp), "read response");
      return resp.Get(sens);
    }
  };
}

int main(int argc, const char* argv[]) {
  if (argc < 3) {
    std::cout << argv[0] << " <device> [test|voltage|current|power|energy]\n";
    return 1;
  }
  try {
    Device dev(argv[1]);
    const std::string mode(argv[2]);
    if (mode == "test") {
      dev.Get(TEST);
      std::cout << "Ok!" << std::endl;
    } else if (mode == "voltage") {
      std::cout << dev.Get(VOLTAGE) << std::endl;
    } else if (mode == "current") {
      std::cout << dev.Get(CURRENT) << std::endl;
    } else if (mode == "power") {
      std::cout << dev.Get(POWER) << std::endl;
    } else if (mode == "energy") {
      std::cout << dev.Get(ENERGY) << std::endl;
    } else {
      Ensure(false, "Unknown mode " + mode);
    }
    return 0;
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
