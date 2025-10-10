#include "AIHasher.h"
#include <Hasher.h>
#include <constants.h>
#include <filesystem>
#include <fstream>
#include <ios>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

std::string markdown_table(const std::vector<std::string> &headers,
                           const std::vector<std::vector<std::string>> &rows) {
  if (headers.empty()) {
    throw std::runtime_error("headers must not be empty");
  }
  std::ostringstream oss;
  oss << '|';
  for (const auto &header : headers) {
    oss << ' ' << header << ' ' << '|';
  }
  oss << '\n';
  oss << '|';
  for (std::size_t i = 0; i < headers.size(); ++i) {
    oss << " ---------- |";
  }
  oss << '\n';
  for (const auto &row : rows) {
    if (row.size() != headers.size()) {
      throw std::runtime_error("row size mismatch");
    }
    oss << '|';
    for (const auto &cell : row) {
      oss << ' ' << cell << ' ' << '|';
    }
    oss << '\n';
  }
  return oss.str();
}

int main() {
  std::vector<std::pair<std::string, std::unique_ptr<IHasher>>> hashers;
  hashers.emplace_back("AIHasher", std::make_unique<AIHasher>());
  hashers.emplace_back("Hasher", std::make_unique<Hasher>());

  const std::vector<std::string> words = {
      "lietuva", "Lietuva", "Lietuva!", "Lietuva!!"};
  std::vector<std::string> sample_inputs;
  for (int length = 1; length <= 64; length *= 4) {
    const char fill = kAlphabet[static_cast<std::size_t>(length % kAlphabet.size())];
    sample_inputs.emplace_back(std::string(static_cast<std::size_t>(length), fill));
  }

  const std::filesystem::path task_result_path = kResultsPath / "task.md";
  std::ofstream oss(task_result_path, std::ios::out);
  if (!oss.is_open()) {
    throw std::runtime_error("failed to open task.md for writing");
  }

  oss << "# Užduotis\n";

  oss << "## 1. Įvedimas – bet kokio ilgio eilutė (string)\n\n";
  std::map<std::string, bool> accepts;
  for (const auto &entry : hashers) {
    accepts[entry.first] = true;
  }
  std::vector<std::string> headers = {"Įvestis"};
  for (const auto &entry : hashers) {
    headers.push_back(entry.first);
  }
  std::vector<std::vector<std::string>> rows;
  for (const auto &input : sample_inputs) {
    std::vector<std::string> row;
    row.push_back(input);
    for (auto &entry : hashers) {
      try {
        row.push_back(entry.second->hash256bit(input));
      } catch (const std::exception &) {
        accepts[entry.first] = false;
        row.push_back("NaN");
      }
    }
    rows.push_back(std::move(row));
  }
  for (const auto &[label, ok] : accepts) {
    oss << label
        << (ok ? " priima betkokio ilgio įvestį\n"
                : " nepriima betkokio ilgio įvesties\n");
  }
  oss << markdown_table(headers, rows) << '\n';

  oss << "\n## 2. Rezultatas – visada vienodo dydžio (256bitai)\n\n";
  std::map<std::string, bool> fixed_length;
  for (const auto &entry : hashers) {
    fixed_length[entry.first] = true;
  }
  headers.clear();
  headers.push_back("Įvestis");
  for (const auto &entry : hashers) {
    headers.push_back(entry.first + " hešo dydis");
  }
  rows.clear();
  for (const auto &input : sample_inputs) {
    std::vector<std::string> row;
    row.push_back(input);
    for (auto &entry : hashers) {
      const std::string hash = entry.second->hash256bit(input);
      if (hash.size() != 64) {
        fixed_length[entry.first] = false;
      }
      row.push_back(std::to_string(hash.size()));
    }
    rows.push_back(std::move(row));
  }
  for (const auto &[label, ok] : fixed_length) {
    oss << label
        << (ok ? " duoda vienodo dydžio hešus\n"
               : " neduoda vienodo dydžio hešų\n");
  }
  oss << markdown_table(headers, rows) << '\n';

  oss << "\n## 3. Deterministiškumas – tas pats įvedimas = tas pats rezultatas\n";
  std::map<std::string, bool> deterministic;
  for (const auto &entry : hashers) {
    deterministic[entry.first] = true;
  }
  headers.clear();
  headers.push_back("Įvestis");
  for (const auto &entry : hashers) {
    headers.push_back(entry.first + " #1");
    headers.push_back(entry.first + " #2");
  }
  rows.clear();
  for (const auto &word : words) {
    std::vector<std::string> row;
    row.push_back(word);
    for (auto &entry : hashers) {
      const std::string first = entry.second->hash256bit(word);
      const std::string second = entry.second->hash256bit(word);
      if (first != second) {
        deterministic[entry.first] = false;
      }
      row.push_back(first);
      row.push_back(second);
    }
    rows.push_back(std::move(row));
  }
  for (const auto &[label, ok] : deterministic) {
    oss << label
        << (ok ? " deterministiškas – ta pati įvestis duoda tą patį rezultatą\n"
               : " nėra deterministiškas – ta pati įvestis gali duoti skirtingą rezultatą\n");
  }
  oss << markdown_table(headers, rows) << '\n';

  oss << "## 4. Efektyvumas – turi veikti pakankamai greitai\n";
  const auto konstitucija_results = kResultsPath / "konstitucija.txt";
  if (std::filesystem::exists(konstitucija_results)) {
    std::ifstream iss(konstitucija_results);
    std::string header_line;
    if (std::getline(iss, header_line)) {
      std::istringstream header_stream(header_line);
      headers.clear();
      std::string token;
      while (header_stream >> token) {
        headers.push_back(token == "Lines" ? "Eilučių kiekis" : token);
      }
      rows.clear();
      std::string line;
      while (std::getline(iss, line)) {
        if (line.empty()) {
          continue;
        }
        std::istringstream line_stream(line);
        std::vector<std::string> row;
        while (line_stream >> token) {
          row.push_back(token);
        }
        if (!row.empty()) {
          rows.push_back(std::move(row));
        }
      }
      if (!rows.empty()) {
        oss << markdown_table(headers, rows) << '\n';
      }
    }
  }

  oss << "\n## 5. Atsparumas kolizijoms – neturi būti lengva (praktiškai labai sudėtinga)\n";

  oss << "\n## 6. Lavinos efektas\n\n";
  headers.clear();
  headers.push_back("Įvestis");
  for (const auto &entry : hashers) {
    headers.push_back(entry.first);
  }
  rows.clear();
  for (const auto &word : words) {
    std::vector<std::string> row;
    row.push_back(word);
    for (auto &entry : hashers) {
      row.push_back(entry.second->hash256bit(word));
    }
    rows.push_back(std::move(row));
  }
  oss << markdown_table(headers, rows) << '\n';

  oss << "\n## 7. Negrįžtamumas – iš hash’o praktiškai neįmanoma atspėti pradinio teksto\n\n";
}