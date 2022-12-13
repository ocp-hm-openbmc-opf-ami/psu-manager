/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <utility.hpp>
#include <xyz/openbmc_project/Control/PowerSupplyRedundancy/server.hpp>

const constexpr char* psuInterface =
    "/xyz/openbmc_project/inventory/system/powersupply/";
const constexpr int oneDay = 86400;
const constexpr int oneMonth = 30 * oneDay;
const constexpr int minRotationPeriod = oneDay;
const constexpr int maxRotationPeriod = 6 * oneMonth;
constexpr const uint8_t pmbusCmdCRSupport = 0xd0;
using Association = std::tuple<std::string, std::string, std::string>;
using crConfigVariant =
    std::variant<bool, uint8_t, uint32_t, std::vector<uint8_t>, std::string>;

class ColdRedundancy
    : sdbusplus::xyz::openbmc_project::Control::server::PowerSupplyRedundancy
{
  public:
    ColdRedundancy(
        boost::asio::io_service& io,
        sdbusplus::asio::object_server& objectServer,
        std::shared_ptr<sdbusplus::asio::connection>& dbusConnection,
        std::vector<std::unique_ptr<sdbusplus::bus::match::match>>& matches);
    ~ColdRedundancy()
    {
        objServer.remove_interface(association);
    };

    uint8_t psuNumber() const override;
    void
        createPSU(boost::asio::io_service& io,
                  sdbusplus::asio::object_server& objectServer,
                  std::shared_ptr<sdbusplus::asio::connection>& dbusConnection);

  private:
    bool crSupported = true;
    bool isRotating = false;
    uint8_t psOrder;
    uint8_t numberOfPSU = 0;
    std::vector<uint8_t> settingsOrder = {};

    void startRotateCR(void);
    void startCRCheck(void);
    void rotateCR(void);
    void configCR(bool reConfig);
    void checkCR(void);
    void reRanking(void);
    void putWarmRedundant(void);
    void keepAliveCheck(void);
    void writePmbus(uint8_t bus, uint8_t slaveAddr, uint8_t value);
    void readPmbus(uint8_t bus, uint8_t slaveAddr, int& value);
    void checkRedundancyEvent(void);
    void saveConfig(void);
    void saveProperty(std::string propertyName, crConfigVariant value);

    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::connection>& systemBus;

    boost::asio::steady_timer timerRotation;
    boost::asio::steady_timer warmRedundantTimer;
    boost::asio::steady_timer timerCheck;
    boost::asio::steady_timer keepAliveTimer;
    boost::asio::steady_timer filterTimer;
    boost::asio::steady_timer puRedundantTimer;

    std::shared_ptr<sdbusplus::asio::dbus_interface> association;
    std::vector<Association> associationsOk;
    std::vector<Association> associationsWarning;
    std::vector<Association> associationsNonCrit;
    std::vector<Association> associationsCrit;
};

class PowerSupply
{
  public:
    PowerSupply(
        std::string& name, uint8_t bus, uint8_t address, uint8_t order,
        const std::shared_ptr<sdbusplus::asio::connection>& dbusConnection);
    ~PowerSupply();
    std::string name;
    uint8_t order = 0;
    uint8_t bus;
    uint8_t address;
    PSUState state = PSUState::normal;
  private:
    void logVersion();
};
