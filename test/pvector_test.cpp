//
// Created by puneet on 23/02/24.
//

#include "pvector.h"

#include <cassert>
#include <iostream>

int main()
{
  // test default constructor
  pvector<int> p1;
  assert(p1.size() == 0);
  assert(p1.capacity() == 0);
  assert(p1.begin() == nullptr);
  assert(p1.end() == nullptr);
  p1.reserve(10);
  assert(p1.capacity() == 10);
  assert(p1.size() == 0);
  p1.push_back(10);
  p1.push_back(20);
  std::cout << p1[0] << std::endl;
  std::cout << p1[1] << std::endl;
  p1.~pvector();

  pvector<int> p(10, 0);
  for (int i = 0; i < 10; i++)
  {
    p.push_back(i);
  }
  for (int i = 0; i < p.size(); i++)
  {
    std::cout << p[i] << std::endl;
  }

  std::cout << "testing fill" << std::endl;
  p.fill(5);
  for (int i = 0; i < 10; i++)
  {
    std::cout << p[i] << std::endl;
  }
  return 0;
}