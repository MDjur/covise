#include "csv.h"

#include <string>

namespace opencover::utils::read {

CSVStream::CSVStream(const std::string &filename, char delimiter)
    : m_filename(filename), m_delimiter(delimiter) {
  m_inputFileStream.open(filename.c_str());
  if (!m_inputFileStream.is_open())
    throw CSVStream_Exception("Could not open file " + filename);
  readHeader();
}

void CSVStream::readHeader() {
  std::string colName("");
  auto ss = getLine();
  while (std::getline(ss, colName, m_delimiter)) m_header.push_back(colName);
}

void CSVStream::readLine(CSVStream::CSVRow &row) {
  std::string value("");
  auto ss = getLine();
  size_t currentColNameIdx = 0;
  for (auto &header : m_header) {
    std::getline(ss, value, m_delimiter);
    // if there is a comma in the value the string will contain \" at the beginning
    // and end of the cell
    if (auto it = value.find('\"'); it != std::string::npos) {
      std::string next = value.substr(it + 1);
      value = next;
      while (next.find('\"') == std::string::npos) {
        std::getline(ss, next, m_delimiter);
        value += "," + next;
      }
      value = value.substr(0, value.size() - 1);
    }

    row[header] = value;
  }
}

std::stringstream CSVStream::getLine() {
  std::getline(m_inputFileStream, m_currentline);
  // skip comments
  while (!m_currentline.empty() && m_currentline[0] == '#')
    std::getline(m_inputFileStream, m_currentline);

  // remove carriage return characters
  if (m_currentline.find('\r') != std::string::npos)
    m_currentline.erase(m_currentline.find('\r'), 1);
  std::stringstream ss(m_currentline);
  return ss;
}
}  // namespace opencover::utils::read
