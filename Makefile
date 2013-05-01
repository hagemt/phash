TAG=CS2HW9

CXXFLAGS=-Wall -Wextra -ggdb -pedantic -std=c++98

CXX=$(shell which g++)
DIFF=$(shell which diff) -s
RM=$(shell which rm) -fv
SAY=$(shell which echo) -e "[$(TAG)]"

all: hw9

clean:
	@$(SAY) "Cleaning generated, object, and executable files..."
	@$(RM) *.pch *.gch
	@$(RM) *.o hw9
	@$(SAY) "Cleaning up temporary test results..."
	@$(RM) chair_test.* chair_diff.pbm
	@$(RM) test.* _.*

test_compress: hw9
	@$(SAY) "Testing deflate..."
	./hw9 compress chair.ppm test.pbm test.ppm test.offset
	@#du -bc chair.ppm && du -bc test.*
	./hw9 uncompress test.pbm test.ppm test.offset chair_test.ppm
	./hw9 compare chair.ppm chair_test.ppm chair_diff.pbm
	@#convert chair_test.ppm chair_test.png && eog chair_test.png

test_uncompress: hw9
	@$(SAY) "Testing inflate..."
	./hw9 uncompress car_occupancy.pbm car_hash_data.ppm car_offset.offset _.ppm
	./hw9 compare car_original.ppm _.ppm _.pbm
	@#convert _.ppm _.png && eog _.png

test: test_uncompress test_compress

.PHONY: all clean test test_compress test_uncompress

hw9: image.o main.o
	@$(SAY) "LINK $@"
	@$(CXX) $(CXXFLAGS) *.o -o $@

%.o: %.cpp
	@$(SAY) "CCXX $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@
