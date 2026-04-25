CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
INCPATH = -I../include

SRCDIR = ../src
SPARSESRC = $(SRCDIR)/icm_core.cpp $(SRCDIR)/icm_sparse_api.cpp

.PHONY: all clean test example

all: example test_all test_complex test_matrix test_sparse test_fft test_spmm_api

example: example_add.cpp $(SRCDIR)/icm_core.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -o example_add example_add.cpp $(SRCDIR)/icm_core.cpp

test_all: test_all.cpp $(SRCDIR)/icm_core.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -o test_all test_all.cpp $(SRCDIR)/icm_core.cpp

test_complex: test_complex.cpp $(SRCDIR)/icm_core.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -o test_complex test_complex.cpp $(SRCDIR)/icm_core.cpp

test_matrix: test_matrix.cpp $(SRCDIR)/icm_core.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -o test_matrix test_matrix.cpp $(SRCDIR)/icm_core.cpp

test_sparse: test_sparse.cpp $(SRCDIR)/icm_core.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -o test_sparse test_sparse.cpp $(SRCDIR)/icm_core.cpp

test_fft: test_fft.cpp $(SRCDIR)/icm_core.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -o test_fft test_fft.cpp $(SRCDIR)/icm_core.cpp

test_spmm_api: test_spmm_api.cpp $(SPARSESRC)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o test_spmm_api test_spmm_api.cpp $(SPARSESRC)

test: test_matrix test_sparse test_spmm_api
	./test_matrix
	./test_sparse
	./test_spmm_api

clean:
	rm -f example_add test_all test_complex test_matrix test_sparse test_fft test_spmm_api