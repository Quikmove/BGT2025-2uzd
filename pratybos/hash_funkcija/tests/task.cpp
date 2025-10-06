#include "sha256_hasher.h"
#include <Hasher.h>
#include <constants.h>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string gen_random(const int len) {
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_s;
  tmp_s.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return tmp_s;
}
std::string
to_markdown_table(const std::vector<std::pair<std::string, std::string>> &poros,
                  std::string column1, std::string column2) {
  std::vector<std::string> entries;
  for (const auto &it : poros) {
    std::ostringstream entry;
    entry << "| " << it.first << " | " << it.second << " |";
    entries.push_back(entry.str());
  }
  std::ostringstream output;
  output << "| " << column1 << " | " << column2 << " |\n";
  output << "| ---------- | ---------- |\n";
  for (const auto &entry : entries) {
    output << entry << '\n';
  }
  return output.str();
}
int main() {
  SHA256_Hasher hasher;
  const auto words =
      std::vector<std::string>{"lietuva", "Lietuva", "Lietuva!", "Lietuva!!"};
  std::filesystem::path taskResultPath = kResultsPath / "task.md";
  std::ofstream oss(taskResultPath, std::ios::out);
  oss << "# Užduotis\n";
  // 1. Įvedimas – bet kokio ilgio eilutė (string)
  oss << "## 1. Įvedimas – bet kokio ilgio eilutė (string)\n\n";
  {
    std::vector<std::pair<std::string, std::string>> poros;
    bool anyLength = true;
    for (int i = 1; i <= 64; i *= 4) {
      std::string input = std::string(i, 'a');
      std::string hash;
      try {
        hash = hasher.hash256bit(input);

      } catch (const std::exception &e) {
        anyLength = false;
        hash = "NaN";
      }
      poros.emplace_back(input, hash);
    }
    if (anyLength) {
      oss << "Hešavimo funkcija priima betkokio ilgio įvestį\n";
    } else {
      oss << "Hešavimo funkcija nepriima betkokio ilgio įvesties\n";
    }
    oss << to_markdown_table(poros, "Įvestis", "Hešas") << '\n';
  }
  // 2. Rezultatas – visada vienodo dydžio (256bitai/64 hex simboliai)
  oss << "\n## 2. Rezultatas – visada vienodo dydžio (256bitai)\n\n";
  {
    int wrong_len_count = 0;
    std::vector<std::pair<std::string, std::string>> poros;
    for (int i = 1; i <= 64; i *= 4) {
      std::string input = std::string(i, 'a');
      std::string output_hash = hasher.hash256bit(input);
      poros.emplace_back(input, std::to_string(output_hash.size()));
      if (output_hash.size() != 64)
        wrong_len_count++;
    }
    oss << to_markdown_table(poros, "Įvestis", "Hešo dydis") << "\n";
    if (wrong_len_count > 0) {
      oss << "Algoritmas neduoda vienodo dydžio hešų - aptikta "
          << wrong_len_count << "ne 64 simbolių ilgio hešų\n";
    } else
      oss << "Algoritmas duoda vienodo dydžio hešus\n";
  }
  // 3. Deterministiškumas – tas pats įvedimas = tas pats rezultatas
  oss << "\n## 3. Deterministiškumas – tas pats įvedimas = tas pats "
         "rezultatas\n";
  {
    auto pairs = std::vector<std::pair<std::string, std::string>>();
    bool deterministic = true;
    for (const auto &word : words) {
      auto hash1 = hasher.hash256bit(word);
      auto hash2 = hasher.hash256bit(word);
      if (hash1 != hash2)
        deterministic = false;
      pairs.emplace_back(word, hash1);
      pairs.emplace_back(word + " (pakartotinai)", hash2);
    }
    if (deterministic) {
      oss << "Algoritmas deterministiškas – ta pati įvestis duoda tą patį "
             "rezultatą\n";
    }

    else {
      oss << "Algoritmas nėra deterministiškas – ta pati įvestis gali duoti "
             "skirtingą rezultatą\n";
    }
    oss << to_markdown_table(pairs, "Įvestis", "Hešas") << '\n';
  }
  // 4. Efektyvumas – turi veikti pakankamai greitai
  oss << "## 4. Efektyvumas – turi veikti pakankamai greitai\n";
  {
    const auto konstitucija_results = kResultsPath / "konstitucija.txt";
    auto pairs = std::vector<std::pair<std::string, std::string>>();
    if (std::filesystem::exists(konstitucija_results)) {
      std::ifstream iss(konstitucija_results);
      std::string lines;
      double time;
      while (iss >> lines >> time) {
        pairs.emplace_back(lines, std::to_string(time));
      }
      oss << "Konstitucija.txt eilučių skaitymo laikai:\n";
      oss << to_markdown_table(pairs, "Eilučių kiekis", "Laikas");
    }
  }

  // 5. Atsparumas kolizijoms – neturi būti lengva (praktiškai labai sudėtinga)
  // rasti du skirtingus įvedimus, kurie duotų tą patį hash’ą
  oss << "\n## 5. Atsparumas kolizijoms – neturi būti lengva (praktiškai labai "
         "sudėtinga)\n";

  // 6. Lavinos efektas
  oss << "\n## 6. Lavinos efektas\n\n";
  {
    auto pairs = std::vector<std::pair<std::string, std::string>>();
    for (const auto &word : words) {
      auto hash = hasher.hash256bit(word);
      pairs.emplace_back(word, hash);
    }
    oss << to_markdown_table(pairs, "Įvestis", "Hešas") << '\n';
  }
  // 7. Negrįžtamumas – iš hash’o praktiškai neįmanoma atspėti pradinio teksto
  oss << "\n## 7. Negrįžtamumas – iš hash’o praktiškai neįmanoma atspėti "
         "pradinio teksto\n\n";
}