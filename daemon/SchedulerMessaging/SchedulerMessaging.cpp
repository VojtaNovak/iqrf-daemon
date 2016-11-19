#include "SchedulerMessaging.h"
#include "DpaTransactionTask.h"
//#include "DpaTask.h"
#include "MqChannel.h"
#include "IDaemon.h"
#include "IqrfLogging.h"

const unsigned IQRF_MQ_BUFFER_SIZE = 1024;

const std::string MQ_ERROR_ADDRESS("ERROR_ADDRESS");
const std::string MQ_ERROR_DEVICE("ERROR_DEVICE");

using namespace std::chrono;

SchedulerMessaging::SchedulerMessaging()
  :m_daemon(nullptr)
  //, m_mqChannel(nullptr)
  //, m_toMqMessageQueue(nullptr)
{
  //m_localMqName = "iqrf-daemon-110";
  //m_remoteMqName = "iqrf-daemon-100";
}

SchedulerMessaging::~SchedulerMessaging()
{
}

void SchedulerMessaging::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
  m_daemon->registerMessaging(*this);
}

void SchedulerMessaging::start()
{
  TRC_ENTER("");

  /*
  m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE, true);

  m_toMqMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    m_mqChannel->sendTo(msg);
  });

  m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });
  */

  m_dpaTaskQueue = ant_new TaskQueue<ScheduleRecord>([&](const ScheduleRecord& record) {
    handleScheduledRecord(record);
  });

  //sec mnt hrs day mon year dow
  std::vector<std::string> temp = {
    "20 30 11 * * * 6",
    "0 * * * * * * mqtt Thermometer 1",
    "5 * * * * * * mqtt PulseLedR 0",
    "10 * * * * * * mqtt PulseLedG 0",
    "15 * * * * * * mqtt PulseLedR 0",
    "20 * * * * * * mqtt PulseLedG 0",
    "25 * * * * * * mqtt PulseLedR 0",
    "30 * * * * * * mqtt PulseLedG 0",
    "35 * * * * * * mqtt PulseLedR 0",
    "40 * * * * * * mqtt PulseLedG 0",
    "45 * * * * * * mqtt PulseLedR 0",
    "50 * * * * * * mqtt PulseLedG 0",
    "55 * * * * * * mqtt PulseLedR 0",
    "00 * * * * * * mqtt PulseLedG 0",
    "00 00 09 10 11 * *",
    "00 00 * * * * *",
    "00 00 00 * * * 1"
  };

  std::vector<std::shared_ptr<ScheduleRecord>> tempRecords;
  int i = 0;
  for (auto & it : temp) {
    tempRecords.push_back(std::shared_ptr<ScheduleRecord>(ant_new ScheduleRecord(it)));
  }

  addScheduleRecords(tempRecords);

  m_scheduledTaskPushed = false;
  m_runTimerThread = true;
  m_timerThread = std::thread(&SchedulerMessaging::timer, this);

  std::cout << "Scheduler started" << std::endl;

  TRC_LEAVE("");
}

void SchedulerMessaging::stop()
{
  TRC_ENTER("");
  /*
  delete m_mqChannel;
  delete m_toMqMessageQueue;
  */

  {
    m_runTimerThread = false;
    std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
    m_scheduledTaskPushed = true;
    m_conditionVariable.notify_one();
  }

  if (m_timerThread.joinable())
    m_timerThread.join();

  delete m_dpaTaskQueue;

  std::cout << "Scheduler stopped" << std::endl;
  TRC_LEAVE("");
}

//void SchedulerMessaging::sendMessageToMq(const std::string& message)
//{
//  ustring msg((unsigned char*)message.data(), message.size());
//  TRC_DBG(FORM_HEX(message.data(), message.size()));
//  m_toMqMessageQueue->pushToQueue(msg);
//}

int SchedulerMessaging::handleScheduledRecord(const ScheduleRecord& record)
{
  TRC_DBG("==================================" << std::endl <<
    "Scheduled msg: " << std::endl << FORM_HEX(record.getTask().data(), record.getTask().size()));

  //get input message
  std::istringstream is(record.getTask());

  //parse input message
  std::string device;
  int address(-1);
  is >> device >> address;

  //to encode output message
  std::ostringstream os;

  //check delivered address
  if (address < 0) {
    os << MQ_ERROR_ADDRESS;
  }
  else if (device == NAME_Thermometer) {
    DpaThermometer temp(address);
    DpaTransactionTask trans(temp);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << temp;
    else
      os << trans.getErrorStr();
  }
  else if (device == NAME_PulseLedG) {
    DpaPulseLedG pulse(address);
    DpaTransactionTask trans(pulse);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << pulse;
    else
      os << trans.getErrorStr();
  }
  else if (device == NAME_PulseLedR) {
    DpaPulseLedR pulse(address);
    DpaTransactionTask trans(pulse);
    m_daemon->executeDpaTransaction(trans);
    int result = trans.waitFinish();

    if (0 == result)
      os << pulse;
    else
      os << trans.getErrorStr();
  }
  else {
    os << MQ_ERROR_DEVICE;
  }

  //sendMessageToMq(os.str());
  {
    std::lock_guard<std::mutex> lck(m_responseHandlersMutex);
    auto found = m_responseHandlers.find(record.getClientId());
    if (found != m_responseHandlers.end()) {
      TRC_DBG(NAME_PAR(Response, os.str()) << " has been passed to: " << NAME_PAR(ClinetId, record.getClientId()));
      found->second(os.str());
      TRC_DBG(NAME_PAR(Response, os.str()) << " has been processed by: " << NAME_PAR(ClinetId, record.getClientId()));
    }
    else {
      TRC_DBG("Response: " << os.str());
    }
  }

  return 0;
}

void SchedulerMessaging::addScheduleRecord(std::shared_ptr<ScheduleRecord>& record)
{
  system_clock::time_point timePoint;
  std::tm timeStr;
  ScheduleRecord::getTime(timePoint, timeStr);
  TRC_DBG(ScheduleRecord::asString(timePoint));

  // lock and insert
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  m_scheduleRecords.push_back(record);
  system_clock::time_point tp = record->getNext(timeStr);
  m_scheduledTasks.insert(std::make_pair(tp, record));

  // notify timer thread
  std::unique_lock<std::mutex> lckn(m_conditionVariableMutex);
  m_scheduledTaskPushed = true;
  m_conditionVariable.notify_one();
}

void SchedulerMessaging::addScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records)
{
  system_clock::time_point timePoint;
  std::tm timeStr;
  ScheduleRecord::getTime(timePoint, timeStr);
  TRC_DBG(ScheduleRecord::asString(timePoint));

  // lock and insert
  std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
  m_scheduleRecords.insert(m_scheduleRecords.end(), records.begin(), records.end());
  for (auto & record : records) {
    system_clock::time_point tp = record->getNext(timeStr);
    m_scheduledTasks.insert(std::make_pair(tp, record));
  }

  // notify timer thread
  std::unique_lock<std::mutex> lckn(m_conditionVariableMutex);
  m_scheduledTaskPushed = true;
  m_conditionVariable.notify_one();
}

//thread function
void SchedulerMessaging::timer()
{
  system_clock::time_point timePoint;
  std::tm timeStr;
  ScheduleRecord::getTime(timePoint, timeStr);
  TRC_DBG(ScheduleRecord::asString(timePoint));

  while (m_runTimerThread) {
    
    { // wait for something in the m_scheduledTasks;
      std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
      m_conditionVariable.wait_until(lck, timePoint, [&]{ return m_scheduledTaskPushed; });
      m_scheduledTaskPushed = false;
    }

    // get actual time
    ScheduleRecord::getTime(timePoint, timeStr);

    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);

    // fire all expired tasks
    while (!m_scheduledTasks.empty()) {

      auto begin = m_scheduledTasks.begin();
      std::shared_ptr<ScheduleRecord> record = begin->second;

      if (begin->first < timePoint) {
        
        // fire
        std::cout << "Task fired at: " << ScheduleRecord::asString(timePoint) << std::endl;
        m_dpaTaskQueue->pushToQueue(*record); //copy record

        // erase fired
        m_scheduledTasks.erase(begin);

        // get and schedule next
        system_clock::time_point nextTimePoint = record->getNext(timeStr);
        if (nextTimePoint >= timePoint) {
          m_scheduledTasks.insert(std::make_pair(nextTimePoint, record));
        }
      }
      else {
        break;
      }
    }

    // get next wakeup time
    if (!m_scheduledTasks.empty()) {
      timePoint = m_scheduledTasks.begin()->first;
    }
    else {
      timePoint += duration<int>(10);
    }

  }
}

void SchedulerMessaging::makeCommand(const std::string& clientId, const std::string& command)
{
}

void SchedulerMessaging::registerResponseHandler(const std::string& clientId, ResponseHandlerFunc fun)
{
  std::lock_guard<std::mutex> lck(m_responseHandlersMutex);
  //TODO check success
  m_responseHandlers.insert(make_pair(clientId, fun));
}

void SchedulerMessaging::unregisterResponseHandler(const std::string& clientId)
{
  std::lock_guard<std::mutex> lck(m_responseHandlersMutex);
  m_responseHandlers.erase(clientId);
}

/////////////////////////////////////////
ScheduleRecord::ScheduleRecord(const std::string& rec)
{
  m_tm.tm_sec = -1;
  m_tm.tm_min = -1;
  m_tm.tm_hour = -1;
  m_tm.tm_mday = -1;
  m_tm.tm_mon = -1;
  m_tm.tm_year = -1;
  m_tm.tm_wday = -1;
  m_tm.tm_yday = -1;
  m_tm.tm_isdst = -1;

  parse(rec);
}

std::vector<std::string> ScheduleRecord::preparseMultiRecord(const std::string& rec)
{
  std::istringstream is(rec);

  std::string sec("*");
  std::string mnt("*");
  std::string hrs("*");
  std::string day("*");
  std::string mon("*");
  std::string yer("*");
  std::string dow("*");

  is >> sec >> mnt >> hrs >> day >> mon >> yer >> dow;
  //TODO
  std::vector<std::string> retval;
  return retval;
}

void ScheduleRecord::parse(const std::string& rec)
{
  std::istringstream is(rec);

  std::string sec("*");
  std::string mnt("*");
  std::string hrs("*");
  std::string day("*");
  std::string mon("*");
  std::string yer("*");
  std::string dow("*");

  is >> sec >> mnt >> hrs >> day >> mon >> yer >> dow >> m_clientId;
  std::getline(is, m_dpaTask, '|'); // just to get rest of line with spaces
  
  m_tm.tm_sec = parseItem(sec, 0, 59);
  m_tm.tm_min = parseItem(mnt, 0, 59);
  m_tm.tm_hour = parseItem(hrs, 0, 23);
  m_tm.tm_mday = parseItem(day, 1, 31);
  m_tm.tm_mon = parseItem(mon, 1, 12) - 1;
  m_tm.tm_year = parseItem(yer, 0, 9000);
  m_tm.tm_wday = parseItem(dow, 0, 6);
}

int ScheduleRecord::parseItem(std::string& item, int mnm, int mxm)
{
  int num;
  try {
    num = std::stoi(item);
    if (num < mnm || num > mxm) {
      num = -1;
    }
  }
  catch (...) {
    num = -1; //TODO
  }
  return num;
}

system_clock::time_point ScheduleRecord::getNext(const std::tm& actualTime)
{
  bool lower = false;
  std::tm c_tm = actualTime;

  while (true) {
    if (!compareTimeVal(c_tm.tm_sec, m_tm.tm_sec, lower))
      break;
    if (!compareTimeVal(c_tm.tm_min, m_tm.tm_min, lower))
      break;
    if (!compareTimeVal(c_tm.tm_hour, m_tm.tm_hour, lower))
      break;
    //Compare Day of Week if present
    if (m_tm.tm_wday >= 0) {
      int dif = c_tm.tm_wday - m_tm.tm_wday;
      if (dif == 0 && !lower)
        c_tm.tm_mday++;
      else if (dif > 0)
        c_tm.tm_mday += 7 - dif;
      else
        c_tm.tm_mday -= dif;
      break;
    }
    if (!compareTimeVal(c_tm.tm_mday, m_tm.tm_mday, lower))
      break;
    if (!compareTimeVal(c_tm.tm_mon, m_tm.tm_mon, lower))
      break;
    if (!compareTimeVal(c_tm.tm_year, m_tm.tm_year, lower))
      break;

    break;
  }

  std::time_t tt = std::mktime(&c_tm);
  if (tt == -1) {
    throw "no valid system time"; //TODO
  }
  
  system_clock::time_point tp = system_clock::from_time_t(tt);
  m_next = asString(tp);
  return tp;
}

//return true - continue false - finish
bool ScheduleRecord::compareTimeVal(int& cval, int tval, bool& lw) const
{
  int dif;
  if (tval >= 0) {
    lw = (dif = cval - tval) < 0 ? true : dif > 0 ? false : lw;
    cval = tval;
    return true;
  }
  else {
    if (!lw) cval++;
    return false;
  }
}

std::string ScheduleRecord::asString(const std::chrono::system_clock::time_point& tp)
{
  // convert to system time:
  std::time_t t = std::chrono::system_clock::to_time_t(tp);
  std::string ts = ctime(&t);   // convert to calendar time
  ts.resize(ts.size() - 1);       // skip trailing newline
  return ts;
}

void ScheduleRecord::getTime(std::chrono::system_clock::time_point& timePoint, std::tm& timeStr)
{
  timePoint = system_clock::now();
  time_t tt;
  tt = system_clock::to_time_t(timePoint);
  std::tm* timeinfo;
  timeinfo = localtime(&tt);
  timeStr = *timeinfo;
}

