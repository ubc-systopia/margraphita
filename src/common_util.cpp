//
// Created by puneet on 12/12/23.
//
#include "common_util.h"

void CommonUtil::create_dir(const std::string &path)
{
  std::filesystem::path dirname = path;
  if (std::filesystem::exists(dirname))
  {
    std::filesystem::remove_all(dirname);  // remove if exists;
  }
  std::filesystem::create_directories(dirname);
}

bool CommonUtil::check_dir_exists(const std::string &path)
{
  if (std::filesystem::exists(path))
  {
    return true;
  }
  else
  {
    return false;
  }
}

void CommonUtil::set_table(WT_SESSION *session,
                           const std::string &prefix,
                           std::vector<std::string> columns,
                           const std::string &key_fmt,
                           const std::string &val_fmt)
{
  if (!columns.empty())
  {
    std::vector<std::string>::iterator ptr;
    std::string concat = columns.at(0);
    for (ptr = columns.begin() + 1; ptr < columns.end(); ptr++)
    {
      concat += "," + *ptr;
    }

    // Now insert in WT
    std::string table_name = "table:" + prefix;
    std::string wt_format_string = "key_format=" + key_fmt +
                                   ",value_format=" + val_fmt + ",columns=(" +
                                   concat + ")";
    char *n = const_cast<char *>(table_name.c_str());
    char *f = const_cast<char *>(wt_format_string.c_str());
    session->create(session, n, f);
  }
  else
  {
    std::string table_name = "table:" + prefix;
    session->create(session, table_name.c_str(), "key_format=I,value_format=I");
  }
}

std::string CommonUtil::get_db_name(const std::string &prefix,
                                    const std::string &name)
{
  return (prefix + "-" + name);
}

void CommonUtil::check_graph_params(const graph_opts &params)
{
  std::vector<std::string> missing_params;

  // TODO: this needs to be updated
  if (!missing_params.empty())
  {
    std::vector<std::string>::iterator missing_param_ptr;
    std::string to_return = missing_params.at(0);
    for (missing_param_ptr = missing_params.begin() + 1;
         missing_param_ptr < missing_params.end();
         missing_param_ptr++)
    {
      to_return += "," + *missing_param_ptr;
    }
    throw GraphException(to_return);
  }
}

int CommonUtil::close_cursor(WT_CURSOR *cursor)
{
  if (int ret = cursor->close(cursor) != 0)
  {
    fprintf(stderr, "Failed to close the cursor\n ");
    return ret;
  }
  return 0;
}

int CommonUtil::close_connection(WT_CONNECTION *conn)
{
  int ret = conn->close(conn, nullptr);
  if (ret != 0)
  {
    throw GraphException("failed to close the connection");
  }
  return 0;
}

int CommonUtil::open_connection(const char *db_name,
                                const std::string &logdir,
                                const std::string &conn_config,
                                WT_CONNECTION **conn)
{
  char config[1024] = "create";  // create if not exists.
  std::string _config;
  _config = conn_config;
#ifdef STAT
  if (_config.length() > 0)
  {
    _config += ",";
  }
  _config +=
      "statistics=(all),statistics_log=(wait=0,on_close=true,path=" + logdir +
      ")";
#endif
  if (!_config.empty())
  {
    snprintf(config + strlen("create"), 1018, ",%s", _config.c_str());
  }
  std::cout << "conn_config is: " << config << std::endl;

  if (wiredtiger_open(db_name, nullptr, config, conn) != 0)
  {
    fprintf(stderr, "Failed to open connection\n");
    return (-1);
  }
  return 0;
}

int CommonUtil::open_session(WT_CONNECTION *conn, WT_SESSION **session)
{
  // if (conn->open_session(conn, NULL, NULL, session) != 0)
  // {
  //     fprintf(stderr, "Failed to open session\n");
  //     return (-1);
  // }
  // return 0;
  if (conn->open_session(conn, nullptr, "isolation=snapshot", session) != 0)
  {
    fprintf(stderr, "Failed to open session\n");
    return (-1);
  }
  return 0;
}

[[maybe_unused]] int CommonUtil::open_cursor(WT_SESSION *session,
                                             WT_CURSOR **cursor,
                                             const std::string &uri,
                                             WT_CURSOR *to_dup,
                                             const std::string &config)
{
  if (session->open_cursor(
          session, uri.c_str(), to_dup, config.c_str(), cursor) != 0)
  {
    fprintf(stderr, "Failed to open the cursor on URI %s", uri.c_str());
  }
  return 0;
}

[[maybe_unused]] int CommonUtil::dup_cursor(WT_SESSION *session,
                                            WT_CURSOR *to_dup,
                                            WT_CURSOR **cursor)
{
  if (session->open_cursor(session, nullptr, to_dup, nullptr, cursor) != 0)
  {
    fprintf(stderr, "Failed to duplicte the cursor on URI %s", to_dup->uri);
  }
  return 0;
}

void CommonUtil::check_return(int retval, const std::string &mesg)
{
  if (retval > 0)
  {
    throw GraphException(mesg);
  }
}
