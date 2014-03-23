OBJ = main.o led_driver.o input_event_driver.o i2c_communication.o variable_loader.o check.o encoder_driver.o our_time.o motor_driver.o

all: ../../build_arm/main

%.o: %.cpp
	g++ -I ../common -c $<

%.o: ../common/%.cpp
	g++ -I ../common -c $<

../../build_arm/main: $(OBJ)
	g++ -o $@ -pthread $(notdir $^)

run: ../../build_arm/main
	cd ../../build_arm/; \
	  sudo ./main || true

clean:
	rm -f *.o
