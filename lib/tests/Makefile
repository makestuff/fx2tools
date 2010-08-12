#
# Copyright (C) 2009-2010 Chris McClelland
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# To run the tests you will need UnitTest++ from http://unittest-cpp.sourceforge.net
#
TARGET = runTests
UTPP_HOME=../../../../3rd/UnitTest++

INCLUDES = \
	-I../../../../include \
	-I../../../../libs/buffer \
	-I../../../../libs/dump

LIBS = \
	../libfx2loader.a \
	../../../../libs/buffer/libbuffer.a \
	../../../../libs/dump/libdump.a \
	$(UTPP_HOME)/libUnitTest++.a

CPP_SRCS = $(shell ls *.cpp)
CPP_OBJS = $(CPP_SRCS:%.cpp=$(OBJDIR)/%.o)
CPP = g++
CPPFLAGS = -O3 -Wall -Wextra -Wundef -std=c++98 -pedantic-errors -DFX2LOADER_PRIVATE -I$(UTPP_HOME)/src $(INCLUDES)
LDFLAGS =
OBJDIR = .build
DEPDIR = .deps

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(CPP_OBJS) $(TEST_OBJS)
	$(CPP) $(LDFLAGS) -o $@ $(CPP_OBJS) $(LIBS)
	strip $(TARGET)

$(OBJDIR)/%.o : %.cpp
	$(CPP) -c $(CPPFLAGS) -MMD -MP -MF $(DEPDIR)/$(@F).d $< -o $@

clean: FORCE
	rm -rf $(OBJDIR) $(TARGET) $(DEPDIR) Debug Release *.ncb *.user tmpFile.*

-include $(shell mkdir -p $(OBJDIR) $(DEPDIR) 2>/dev/null) $(wildcard $(DEPDIR)/*)

FORCE:
