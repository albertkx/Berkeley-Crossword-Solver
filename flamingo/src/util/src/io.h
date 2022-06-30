/*
  $Id: io.h 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD
  license.

  Date: 06/27/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _io_h_
#define _io_h_

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "range.h"
#include "debug.h"

template<class T> 
class out
{
  T data;
public:
  out(T data = T()):
    data(data)
  {}
  
  operator T() const { return data; }
  
  friend std::ostream& operator<<(std::ostream &output, const out &b) 
  {
    return output << b.data;
  }
};

// -- - read/write a binary string from/to stream - --
std::istream& read_binary_string(std::istream &in, std::string &str);
std::ostream& write_binary_string(std::ostream &out, const std::string &str);

// -- - for use with copy-like functions for read/write - -- 
// e.g.,  copy(begin, end, ostream_iterator<binary_io<type> >(file));
template<class T>
class binary_io
{
  T data;
public:
  binary_io(T data = T()):
    data(data)
  {}
  
  operator T() const { return data; }
  
  friend std::istream& operator>>(std::istream &in, binary_io &b)
  {
    return in.read(reinterpret_cast<char*>(&b.data), sizeof(T));
  }

  friend std::ostream& operator<<(std::ostream &out, const binary_io &b) 
  {
    return out.write(reinterpret_cast<const char*>(&b.data), sizeof(T));
  }
};

template<>
class binary_io<std::string>
{
  std::string data;
public:
  binary_io(std::string data = std::string()):
    data(data)
  {}
  
  operator std::string() const { return data; }
  
  friend std::istream& operator>>(std::istream &in, binary_io &b)
  {
    return read_binary_string(in, b.data);
  }

  friend std::ostream& operator<<(std::ostream &out, const binary_io &b) 
  {
    return write_binary_string(out, b.data);
  }
};

template<class T>
class binary_io<range<T> >
{
  range<T> data;
public:
  binary_io(range<T> value = range<T>()):
    data(value)
  {}
  
  operator range<T>() const { return data; }
  
  friend std::istream& operator>>(std::istream &in, binary_io &b)
  {
    in.read(reinterpret_cast<char*>(&b.data._begin), sizeof(T));
    return in.read(reinterpret_cast<char*>(&b.data.end), sizeof(T));
  }

  friend std::ostream& operator<<(std::ostream &out, const binary_io &b) 
  {
    out.write(reinterpret_cast<const char*>(&b.data._begin), sizeof(T));
    return out.write(reinterpret_cast<const char*>(&b.data._end), sizeof(T));
  }
};

template<class T>
std::istream& text_io_read(std::istream &in, T &d)
{
  return in >> d;
}

template<>
std::istream& text_io_read(std::istream &in, std::string &d);

template <class T>
class text_io
{
  T data;
public:
  text_io(T value = T()):
    data(value)
  {}
  
  operator T() const { return data; }
  

  friend std::istream& operator>>(std::istream &in, text_io &b)
  {
    return text_io_read(in, b.data);
  }

  friend std::ostream& operator<<(std::ostream &out, const text_io &b) 
  {
    return out << b.data << std::endl;
  }
};

template<class T> 
struct text_reader
{
  std::istream &in;
  
  text_reader(std::istream &in):
    in(in)
  {}

  void operator()(T& e)
  {
    in >> e;
  }
};

// -- - for use with foreach-like functions for read - --
// usefull when we don't want to read the entire file
// we do as many reds as elements in the container
// e.g., for_each(begin, end, binary_reader<type>(file))
template<class T> 
struct binary_reader
{
  std::istream &in;
  
  binary_reader(std::istream &in):
    in(in)
  {}

  void operator()(T &e)
  {
    in.read(reinterpret_cast<char*>(&e), sizeof(T));
  }
};

template<> 
struct binary_reader<std::string>
{
  std::istream &in;
  
  binary_reader(std::istream &in):
    in(in)
  {}

  void operator()(std::string &e)
  {
    read_binary_string(in, e);
  }
};

template<class T> 
struct binary_reader<range<T> >
{
  std::istream &in;
  
  binary_reader(std::istream &in):
    in(in)
  {}

  void operator()(range<T> &e)
  {
    in.read(reinterpret_cast<char*>(&e._begin), sizeof(T));
    in.read(reinterpret_cast<char*>(&e._end), sizeof(T));
  }
};

template<class T> 
struct binary_writer
{
  std::ostream &out;
  
  binary_writer(std::ostream &out):
    out(out)
  {}

  void operator()(T &e)
  {
    out.write(reinterpret_cast<char*>(&e), sizeof(T));
  }
};

template<class T, class OutputIterator> 
void readTextFile(const std::string &filename, OutputIterator out)
{
  std::ifstream file(filename.c_str(), std::ios::in);

  if (!file) {
    std::cerr << "can't open input file \"" << filename << "\"" << std::endl;
    exit(EXIT_FAILURE);
  }

  READING_FILE(filename);
  copy(
    std::istream_iterator<text_io<T> >(file), 
    std::istream_iterator<text_io<T> >(), 
    out);
  READING_DONE();
}

template<class T, class OutputIterator> 
void readBinaryFile(const std::string &filename, OutputIterator out)
{
  std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);

  if (!file) {
    std::cerr << "can't open input file \"" << filename << "\"" << std::endl;
    exit(EXIT_FAILURE);
  }

  READING_FILE(filename);
  copy(
    std::istream_iterator<binary_io<T> >(file), 
    std::istream_iterator<binary_io<T> >(), 
    out);
  READING_DONE();
}

template<class T, class InputIterator> 
void writeTextFile(
  const std::string &filename, 
  InputIterator in, 
  InputIterator in_end)
{
  std::ofstream file(filename.c_str(), std::ios::out);

  if (!file) {
    std::cerr << "can't open output file \"" << filename << "\"" << std::endl;
    exit(EXIT_FAILURE);
  }

  WRITING_FILE(filename);
  copy(in, in_end, std::ostream_iterator<text_io<T> >(file));
  WRITING_DONE();
}

template<class T, class InputIterator> 
void writeBinaryFile(
  const std::string &filename, 
  InputIterator in, 
  InputIterator in_end)
{
  std::ofstream file(filename.c_str(), std::ios::out | std::ios::binary);

  if (!file) {
    std::cerr << "can't open output file \"" << filename << "\"" << std::endl;
    exit(EXIT_FAILURE);
  }

  WRITING_FILE(filename);
  copy(in, in_end, std::ostream_iterator<binary_io<T> >(file));
  WRITING_DONE();
}

// -- - more code - --
/*
  bool existFile(const string &filename) 
  {
  std::ifstream file(filename.c_str(), std::ios::in);
  if (!file) 
  return false;
  file.close();
  return true;
  }
*/

/*
template<class T> 
class binary_input_iterator: public std::iterator<std::input_iterator_tag, T> 
{
private:
  std::istream *i_ptr;
  T d;

public:
  binary_input_iterator():
    i_ptr(0)
  {}
  
  binary_input_iterator(std::istream& i):
    i_ptr(&i)
  { read(); }

  bool operator==(const binary_input_iterator<T> &j) const
  {
    return
      i_ptr == j.i_ptr || 
      i_ptr == 0 && (! *j.i_ptr || j.i_ptr->eof()) || 
      (! *i_ptr || i_ptr->eof()) && j.i_ptr == 0;
  }
  
  bool operator!=(const binary_input_iterator<T> &j) const 
  { return !operator==(j); }

  const binary_input_iterator<T>& operator++() { read(); return *this; }

  binary_input_iterator<T> operator++(int) { read(); return *this; }

  T operator*() { return d; }
  
private:
  void read() { i_ptr->read(reinterpret_cast<char*>(&d), sizeof(T)); }
};

template<class T> 
class binary_output_iterator: public std::iterator<std::output_iterator_tag, T> 
{
  std::ostream *o_ptr;

public:
  binary_output_iterator(std::ostream& i):
    o_ptr(&i)
  {}

  const binary_output_iterator<T>& operator++() const { return *this; }

  binary_output_iterator<T>& operator*() { return *this; }

  binary_output_iterator<T>& operator=(const T & d)
  {
    o_ptr->write(reinterpret_cast<const char*>(&d), sizeof(T));
    return *this;
  }
};
*/

#endif  // _io_h_
