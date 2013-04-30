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
	@$(RM) car_test.* car_diff.pbm
	@$(RM) test.* _.*

test: hw9
	./hw9 compress   car_original.ppm test.pbm test.ppm test.offset
	./hw9 uncompress test.pbm test.ppm test.offset car_test.ppm
	./hw9 compare    car_original.ppm car_test.ppm car_diff.pbm
	convert          car_test.ppm car_test.png && eog car_test.png
	./hw9 uncompress car_occupancy.pbm car_hash_data.ppm car_offset.offset _.ppm
	./hw9 compare    car_original.ppm _.ppm _.pbm
	convert          _.ppm _.png && eog _.png

.PHONY: all clean test

hw9: image.o main.o
	@$(SAY) "LINK $@"
	@$(CXX) $(CXXFLAGS) *.o -o $@

%.o: %.cpp
	@$(SAY) "CCXX $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@
