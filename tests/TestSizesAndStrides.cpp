#include <catch2/catch_test_macros.hpp>
#include "SizesAndStrides.hpp"

TEST_CASE("SizesAndStrides - Default Construction", "[SizesAndStrides]") {
    mg::SizesAndStrides ss;

    // Default rank is initialized to 1[cite: 1]
    REQUIRE(ss.Rank() == 1);
    
    // Default size is 0, default stride is 1[cite: 1]
    REQUIRE(ss.SizeAt(0) == 0);
    REQUIRE(ss.StrideAt(0) == 1);
}

TEST_CASE("SizesAndStrides - Modification and Accessors", "[SizesAndStrides]") {
    mg::SizesAndStrides ss;
    ss.Resize(2); // Sets rank to 2[cite: 1]

    ss.SizeAt(0) = 4;
    ss.SizeAt(1) = 5;
    ss.StrideAt(0) = 5;
    ss.StrideAt(1) = 1;

    REQUIRE(ss.SizeAt(0) == 4);
    REQUIRE(ss.SizeAt(1) == 5);
    REQUIRE(ss.StrideAt(0) == 5);
    REQUIRE(ss.StrideAt(1) == 1);

    SECTION("Iterator access") {
        std::size_t expected_sizes[] = {4, 5};
        std::size_t expected_strides[] = {5, 1};

        std::size_t idx = 0;
        for (auto it = ss.SizesBegin(); it != ss.SizesEnd(); ++it) {
            REQUIRE(*it == expected_sizes[idx++]);
        }

        idx = 0;
        for (auto it = ss.StridesBegin(); it != ss.StridesEnd(); ++it) {
            REQUIRE(*it == expected_strides[idx++]);
        }
    }
}

TEST_CASE("SizesAndStrides - Copy Construction and Assignment", "[SizesAndStrides]") {
    mg::SizesAndStrides original;
    original.Resize(2);
    original.SizeAt(0) = 10;
    original.SizeAt(1) = 20;
    original.StrideAt(0) = 20;
    original.StrideAt(1) = 1;

    SECTION("Copy Constructor") {
        mg::SizesAndStrides copy(original); //[cite: 1]
        REQUIRE(copy == original);
        REQUIRE(copy.Rank() == original.Rank());
        REQUIRE(copy.SizeAt(0) == 10);
    }

    SECTION("Copy Assignment") {
        mg::SizesAndStrides assigned;
        assigned = original; //[cite: 1]
        REQUIRE(assigned == original);
        REQUIRE(assigned.Rank() == original.Rank());
        REQUIRE(assigned.SizeAt(1) == 20);
    }
}

TEST_CASE("SizesAndStrides - Move Construction and Assignment", "[SizesAndStrides]") {
    mg::SizesAndStrides original;
    original.Resize(2);
    original.SizeAt(0) = 3;
    original.SizeAt(1) = 6;

    SECTION("Move Constructor") {
        mg::SizesAndStrides moved(std::move(original)); //[cite: 1]
        REQUIRE(moved.Rank() == 2);
        REQUIRE(moved.SizeAt(0) == 3);
        REQUIRE(moved.SizeAt(1) == 6);
        // Source object rank becomes 0 after move[cite: 1]
        REQUIRE(original.Rank() == 0);
    }

    SECTION("Move Assignment") {
        mg::SizesAndStrides moved;
        moved = std::move(original); //[cite: 1]
        REQUIRE(moved.Rank() == 2);
        REQUIRE(moved.SizeAt(0) == 3);
        REQUIRE(moved.SizeAt(1) == 6);
        // Source object rank becomes 0 after move assignment[cite: 1]
        REQUIRE(original.Rank() == 0);
    }
}

TEST_CASE("SizesAndStrides - Equality Operators", "[SizesAndStrides]") {
    mg::SizesAndStrides first;
    mg::SizesAndStrides second;

    REQUIRE(first == second);

    first.SizeAt(0) = 8;
    REQUIRE(first != second);

    second.SizeAt(0) = 8;
    REQUIRE(first == second);

    second.Resize(2);
    REQUIRE(first != second);
}

TEST_CASE("SizesAndStrides - Resizing Behavior", "[SizesAndStrides]") {
    mg::SizesAndStrides ss;

    SECTION("Expanding Rank zeros out new dimensions") {
        ss.SizeAt(0) = 12;
        ss.StrideAt(0) = 1;
        
        ss.Resize(3); //[cite: 1]
        REQUIRE(ss.Rank() == 3);
        
        // Existing values remain
        REQUIRE(ss.SizeAt(0) == 12);
        REQUIRE(ss.StrideAt(0) == 1);
        
        // New values are zeroed[cite: 1]
        REQUIRE(ss.SizeAt(1) == 0);
        REQUIRE(ss.SizeAt(2) == 0);
        REQUIRE(ss.StrideAt(1) == 0);
        REQUIRE(ss.StrideAt(2) == 0);
    }

    SECTION("Shrinking Rank zeros out previously occupied memory") {
        ss.Resize(3);
        ss.SizeAt(2) = 99;
        ss.StrideAt(2) = 42;

        ss.Resize(1); //[cite: 1]
        REQUIRE(ss.Rank() == 1);

        // Expand again to ensure trimmed positions were zeroed out[cite: 1]
        ss.Resize(3);
        REQUIRE(ss.SizeAt(2) == 0);
        REQUIRE(ss.StrideAt(2) == 0);
    }
}