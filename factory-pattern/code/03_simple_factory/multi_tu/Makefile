# 03 多 TU 实验：编译依赖面实测
# 用法：bash run.sh
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -MMD -MP -I.
SRCS     = temp_sensor.cpp pressure_sensor.cpp sensor_factory.cpp \
           business.cpp business_old.cpp main.cpp
OBJS     = $(SRCS:.cpp=.o)
DEPS     = $(OBJS:.o=.d)
OUTDIR   = $(if $(TMPDIR),$(TMPDIR),/tmp)
BIN      = $(OUTDIR)/fp03_multi_tu

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(OBJS) -o $@

# 自动依赖：gcc 生成的 .d 文件记录每个 .o 依赖哪些头
-include $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(BIN)

.PHONY: all clean
