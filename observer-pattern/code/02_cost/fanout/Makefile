# 02_cost/fanout —— 量化「改动落在谁身上」：一个公共头和它的依赖面
#
#   make          全建
#   make clean    清掉 build/
#
# 产物全部落在 build/（仓库根 .gitignore 已忽略），不在源码目录留垃圾。
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O1 -Wall -MMD -MP

BUILD  := build
SRCS   := main.cpp a.cpp b.cpp c.cpp
OBJS   := $(addprefix $(BUILD)/,$(SRCS:.cpp=.o))
DEPS   := $(OBJS:.o=.d)
TARGET := $(BUILD)/fanout

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CXX) $(OBJS) -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(BUILD)
	@echo "  CXX $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@rm -rf $(BUILD)

.PHONY: all clean
