#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <uv.h>

#include "log.h"

using std::cerr;
using std::cout;
using std::dec;
using std::endl;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::setw;
using std::strerror;
using std::string;
using std::to_string;

class NullLogger : public Logger
{
public:
  NullLogger() = default;

  Logger *prefix(const char * /*file*/, int /*line*/) override { return this; }

  ostream &stream() override { return unopened; }

private:
  ofstream unopened;
};

class FileLogger : public Logger
{
public:
  FileLogger(const char *filename) : log_stream{filename, std::ios::out | std::ios::app}
  {
    if (!log_stream) {
      errno_t stream_errno = errno;

      ostringstream msg;
      msg << "Unable to log to " << filename << ": " << strerror(stream_errno);
      err = msg.str();
    }

    FileLogger::prefix(__FILE__, __LINE__);
    log_stream << "FileLogger opened." << endl;
  }

  Logger *prefix(const char *file, int line) override
  {
    log_stream << "[" << setw(15) << file << ":" << setw(3) << dec << line << "] ";
    return this;
  }

  ostream &stream() override { return log_stream; }

  string get_error() const override { return err; }

private:
  ofstream log_stream;
  string err;
};

class StderrLogger : public Logger
{
public:
  StderrLogger()
  {
    StderrLogger::prefix(__FILE__, __LINE__);
    cerr << "StderrLogger opened." << endl;
  }

  Logger *prefix(const char *file, int line) override
  {
    cerr << "[" << setw(15) << file << ":" << setw(3) << dec << line << "] ";
    return this;
  }

  ostream &stream() override { return cerr; }

  string get_error() const override
  {
    if (!cerr) {
      return "Unable to log to stderr";
    }

    return "";
  }
};

class StdoutLogger : public Logger
{
public:
  StdoutLogger()
  {
    StdoutLogger::prefix(__FILE__, __LINE__);
    cout << "StdoutLogger opened." << endl;
  }

  Logger *prefix(const char *file, int line) override
  {
    cout << "[" << setw(15) << file << ":" << setw(3) << dec << line << "] ";
    return this;
  }

  ostream &stream() override { return cout; }

  string get_error() const override
  {
    if (!cout) {
      return "Unable to log to stdout";
    }

    return "";
  }
};

static uv_key_t current_logger_key;
static NullLogger the_null_logger;
static uv_once_t make_key_once = UV_ONCE_INIT;

static void make_key()
{
  uv_key_create(&current_logger_key);
}

Logger *Logger::current()
{
  uv_once(&make_key_once, &make_key);

  auto *logger = static_cast<Logger *>(uv_key_get(&current_logger_key));

  if (logger == nullptr) {
    uv_key_set(&current_logger_key, static_cast<void *>(&the_null_logger));
    logger = &the_null_logger;
  }

  return logger;
}

string replace_logger(const Logger *new_logger)
{
  string r = new_logger->get_error();
  if (r != "") return r;

  Logger *prior = Logger::current();
  if (prior != &the_null_logger) {
    delete prior;
  }

  uv_key_set(&current_logger_key, (void *) new_logger);
  return "";
}

string Logger::to_file(const char *filename)
{
  return replace_logger(new FileLogger(filename));
}

string Logger::to_stderr()
{
  return replace_logger(new StderrLogger());
}

string Logger::to_stdout()
{
  return replace_logger(new StdoutLogger());
}

string Logger::disable()
{
  return replace_logger(&the_null_logger);
}

string plural(long quantity, const string &singular_form, const string &plural_form)
{
  string result;
  result += to_string(quantity);
  result += " ";

  if (quantity == 1) {
    result += singular_form;
  } else {
    result += plural_form;
  }

  return result;
}

string plural(long quantity, const string &singular_form)
{
  string plural_form(singular_form + "s");
  return plural(quantity, singular_form, plural_form);
}
