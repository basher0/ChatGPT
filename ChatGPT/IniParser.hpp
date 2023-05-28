/*
    Simple header only parser for INI files.

    Source: https://github.com/virtmax/iniparser



    The MIT License

    Copyright 2022 Maxim Singer

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software
    and associated documentation files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
    NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
    THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>



class IniParser
{
public:
    IniParser() : iniString(""), filePath("")
    {}

    bool parseFile(std::string filePath)
    {
        std::fstream file;
        file.open(filePath);

        if (!file.is_open())
            return false;

        this->filePath = filePath;

        iniString = std::string((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        if (file.is_open())
            file.close();

        return parse(iniString);
    }


    bool parse(const std::string& initFileText)
    {
        std::vector<std::string> lines = tokenize(initFileText, "\n");
        std::string currentSection = "";

        const std::regex patternSection("^\\s*\\[\\s*(.*\\S)\\s*\\]\\s*(?:[#;].*)?$");
        const std::regex patternKeyEqualValue("^\\s*(?:\"(.+)\"|(.*[^\\s\"]))\\s*=\\s*(?:\"(.*)\"|([^#;\\[]*\\S))\\s*([#;].*)?$");
        const std::regex patternKeyEqualList("^\\s*(?:\"(.+)\"|(.*[^\\s\"]))\\s*=\\s*(?:\"(.*)\"|([^#;]*\\[.*\\]))\\s*([#;].*)?$");

        LineEntry lineEntry;
        for (size_t i = 0; i < lines.size(); i++)
        {
            lines[i].erase(0, lines[i].find_first_not_of(" \t\n\r\f\v"));   // trim leading whitespaces

            std::smatch match;

            lineEntry.type = LineEntry::TYPE::Comment;
            lineEntry.content = lines[i];

            if (lines[i].size() == 0)
            {
                lineEntry.type = LineEntry::TYPE::Empty;
            }
            else if (std::regex_search(lines[i], match, patternSection))
            {
                currentSection = match[1].str();
                lineEntry.type = LineEntry::TYPE::Section;
                lineEntry.content = currentSection;
            }
            else if (std::regex_search(lines[i], match, patternKeyEqualValue))
            {
                const std::string& key = match[1].str().empty() ? match[2].str() : match[1].str();
                const std::string& value = match[3].str().empty() ? match[4].str() : match[3].str();
                storage[currentSection].entries[key] = value;
                storage[currentSection].originalParsed++;
                lineEntry.type = LineEntry::TYPE::Key;
                lineEntry.content = key;
                lineEntry.value = value;
            }
            else if (std::regex_search(lines[i], match, patternKeyEqualList))
            {
                const std::string& key = match[1].str().empty() ? match[2].str() : match[1].str();
                std::string value = match[3].str().empty() ? match[4].str() : match[3].str();

                // remove brackets []
                value.erase(0, value.find_first_not_of(" ["));
                value.erase(value.find_last_not_of(" ]") + 1, value.size() - 1);

                auto strlist = tokenize(value, ",");
                std::vector<double> doublelist;
                for (const auto& s : strlist)
                {
                    doublelist.emplace_back(std::stod(s));
                }

                storage[currentSection].entries[key] = doublelist;
                storage[currentSection].originalParsed++;
                lineEntry.type = LineEntry::TYPE::Key;
                lineEntry.content = key;
                lineEntry.value = value;
            }

            this->fileLines.push_back(lineEntry);
        }

        return true;
    }


    std::string str()
    {
        std::stringstream sstream;

        std::map<std::string, size_t> keysCounter;
        std::string sectionName = "";
        for (auto& line : fileLines)
        {
            if (line.type == LineEntry::TYPE::Empty)
                sstream << std::endl;
            else if (line.type == LineEntry::TYPE::Comment)
                sstream << line.content << std::endl;
            else if (line.type == LineEntry::TYPE::Key)
            {
                auto& val = storage[sectionName].entries[line.content];
                sstream << line.content << " = " << val << std::endl;
                val.saved = true;

                if (keysCounter.count(sectionName) == 0)
                    keysCounter[sectionName] = 0;
                else
                    keysCounter[sectionName]++;
            }
            else if (line.type == LineEntry::TYPE::Section)
            {
                sstream << "[" << line.content << "]" << std::endl;

                sectionName = line.content;
                keysCounter[sectionName] = 0;
            }

            if (storage[sectionName].originalParsed == keysCounter[sectionName])
            {
                // go throuth elements added since last parse
                for (auto& keyVal : storage[sectionName].entries)
                {
                    if (keyVal.second.saved == false)
                    {
                        sstream << keyVal.first << " = " << keyVal.second << std::endl;
                        keyVal.second.saved = true;
                    }
                }
            }
        }

        // for empty files
        for (auto& section : storage)
        {
            sectionName = section.first;
            if (storage[sectionName].originalParsed == keysCounter[sectionName])
            {
                // go throuth elements added since last parse
                for (auto& keyVal : storage[sectionName].entries)
                {
                    if (keyVal.second.saved == false)
                    {
                        sstream << keyVal.first << " = " << keyVal.second << std::endl;
                        keyVal.second.saved = true;
                    }
                }
            }
        }

        return sstream.str();
    }


    bool save(std::string filePath = "")
    {
        if (filePath.length() == 0)
        {
            filePath = this->filePath;
        }

        std::fstream file;
        file.open(filePath, std::fstream::out);

        if (!file.is_open())
            return false;

        setAllValuesToNotSaved();

        file << str();

        if (file.is_open())
            file.close();

        return true;
    }

    struct ValueVariant : public std::string
    {
        template<typename T>
        ValueVariant(const T& t) : std::string(std::to_string(t))
        {}

        template<size_t N>
        ValueVariant(const char(&s)[N]) : std::string(s, N)
        {}

        ValueVariant(const char* charstr) : std::string(charstr)
        {}

        ValueVariant(const std::string& str = std::string()) : std::string(str)
        {}

        ValueVariant(const std::vector<double>& dlist)
        {
            std::string str = "[";
            for (const auto& v : dlist)
            {
                str += std::to_string(v) + ", ";
            }
            str.erase(str.size() - 3, str.size() - 1);
            str += "]";
            *this = str;
        }

        template<typename T>
        operator T() const
        {
            T t;
            std::stringstream ss;
            return ss << *this && (ss >> t) ? t : T();
        }

        template<typename T>
        bool operator ==(const T& t) const
        {
            return this->compare(ValueVariant(t)) == 0;
        }

        bool operator ==(const char* t) const
        {
            return this->compare(t) == 0;
        }

        bool saved{ false };
    };

    ValueVariant& operator[](const std::string& sectionDotKey)
    {
        std::smatch match;
        if (std::regex_match(sectionDotKey, match, std::regex("^.+\\..+$")))
        {
            auto tokens = tokenize(sectionDotKey, ".");
            return storage[tokens[0]].entries[tokens[1]];
        }
        else if (std::regex_match(sectionDotKey, match, std::regex("^.+$")))
        {
            return storage[""].entries[sectionDotKey];
        }
        else
            throw std::invalid_argument("IniParser::operator[](const std::string& section_dot_key): invalid node_dot_key format. "
                "Should be 'section.key' or 'key' for global keys without a section.");
    }

    std::vector<double> getList(const std::string& sectionDotKey)
    {
        std::vector<double> dlist;
        std::string value = operator [](sectionDotKey);

        value.erase(0, value.find_first_not_of(" ["));
        value.erase(value.find_last_not_of(" ]") + 1, value.size() - 1);

        auto strlist = tokenize(value, ",");
        for (const auto& s : strlist)
        {
            dlist.emplace_back(std::stod(s));
        }

        return dlist;
    }

private:
    std::vector<std::string> tokenize(const std::string& str, const std::string& chars) const
    {
        std::vector<std::string> tokens;
        size_t i = 0, j = 0;
        for (; i < str.size(); i++)
        {
            for (auto& delim : chars)
            {
                if (str[i] == delim)
                {
                    tokens.push_back(str.substr(j, i - j));
                    j = i + 1;
                    break;
                }
            }
        }
        tokens.push_back(str.substr(j, i - j));
        return tokens;
    }

    void setAllValuesToNotSaved()
    {
        for (auto& section : storage)
        {
            for (auto& keyVal : section.second.entries)
            {
                keyVal.second.saved = false;
            }
        }
    }

    struct LineEntry
    {
        enum TYPE { Empty, Comment, Section, Key };

        TYPE type;
        std::string content;
        ValueVariant value;
    };

    std::vector<LineEntry> fileLines;

    struct SectionEntries
    {
        size_t originalParsed{ 0 };
        std::map<std::string, ValueVariant> entries;
    };

    std::map<std::string, SectionEntries> storage;
    std::string iniString;

    std::string filePath;
};