#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "processing_unit_server.hpp"
#include <thread>

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

std::thread CreateServerThread(int port) {
    return std::thread([&]() {
		ProcessingUnit::ProcessingUnitServer(port).StartProcessingUnitServer();
	});
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    auto server_thread = CreateServerThread(8001);
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}