project(DINING_PHILOSOPHERS)

add_executable(DiningPhilosophers DiningPhilosophers.cpp)

target_link_libraries(DiningPhilosophers boost_thread)