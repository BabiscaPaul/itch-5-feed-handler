#include <gtest/gtest.h>
#include "order_book.h"

TEST(OrderBookTest, EmptyBookReturnsNoBBO) {
    OrderBook book;
    EXPECT_FALSE(book.get_bbo().has_value());
}

TEST(OrderBookTest, OnlyBidsReturnsNoBBO) {
    OrderBook book;
    book.add(Side::Buy, 1650000, 100);
    EXPECT_FALSE(book.get_bbo().has_value());
}

TEST(OrderBookTest, OnlyAsksReturnsNoBBO) {
    OrderBook book;
    book.add(Side::Sell, 1651000, 75);
    EXPECT_FALSE(book.get_bbo().has_value());
}

TEST(OrderBookTest, SingleBidAndAskBBOMatches) {
    OrderBook book;
    book.add(Side::Buy,  1650000, 100);
    book.add(Side::Sell, 1651000, 75);

    auto bbo = book.get_bbo();
    ASSERT_TRUE(bbo.has_value());
    EXPECT_EQ(bbo->bid.price,  1650000);
    EXPECT_EQ(bbo->bid.shares, 100);
    EXPECT_EQ(bbo->ask.price,  1651000);
    EXPECT_EQ(bbo->ask.shares, 75);
}

TEST(OrderBookTest, BestBidIsHighestPrice) {
    OrderBook book;
    book.add(Side::Buy, 1640000, 50);
    book.add(Side::Buy, 1650000, 100);
    book.add(Side::Buy, 1645000, 75);
    book.add(Side::Sell, 1660000, 10);

    auto bbo = book.get_bbo();
    ASSERT_TRUE(bbo.has_value());
    EXPECT_EQ(bbo->bid.price, 1650000);
    EXPECT_EQ(bbo->bid.shares, 100);
}

TEST(OrderBookTest, BestAskIsLowestPrice) {
    OrderBook book;
    book.add(Side::Buy,  1640000, 50);
    book.add(Side::Sell, 1660000, 200);
    book.add(Side::Sell, 1655000, 100);
    book.add(Side::Sell, 1670000, 300);

    auto bbo = book.get_bbo();
    ASSERT_TRUE(bbo.has_value());
    EXPECT_EQ(bbo->ask.price, 1655000);
    EXPECT_EQ(bbo->ask.shares, 100);
}

TEST(OrderBookTest, AddAccumulatesAtSameLevel) {
    OrderBook book;
    book.add(Side::Buy, 1650000, 100);
    book.add(Side::Buy, 1650000, 50);
    book.add(Side::Sell, 1660000, 1);

    auto bbo = book.get_bbo();
    ASSERT_TRUE(bbo.has_value());
    EXPECT_EQ(bbo->bid.price, 1650000);
    EXPECT_EQ(bbo->bid.shares, 150);
}

TEST(OrderBookTest, ReducePartialKeepsLevel) {
    OrderBook book;
    book.add(Side::Buy,  1650000, 100);
    book.add(Side::Sell, 1660000, 50);
    book.reduce(Side::Buy, 1650000, 30);

    auto bbo = book.get_bbo();
    ASSERT_TRUE(bbo.has_value());
    EXPECT_EQ(bbo->bid.price,  1650000);
    EXPECT_EQ(bbo->bid.shares, 70);
}

TEST(OrderBookTest, ReduceToZeroErasesLevel) {
    OrderBook book;
    book.add(Side::Buy, 1640000, 50);
    book.add(Side::Buy, 1650000, 100);
    book.add(Side::Sell, 1660000, 10);

    book.reduce(Side::Buy, 1650000, 100);

    auto bbo = book.get_bbo();
    ASSERT_TRUE(bbo.has_value());
    EXPECT_EQ(bbo->bid.price,  1640000);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(OrderBookTest, ReduceLastLevelEmptiesSide) {
    OrderBook book;
    book.add(Side::Buy,  1650000, 100);
    book.add(Side::Sell, 1660000, 50);

    book.reduce(Side::Buy, 1650000, 100);

    EXPECT_FALSE(book.has_bids());
    EXPECT_FALSE(book.get_bbo().has_value());
}

TEST(OrderBookTest, HasBidsHasAsksOnEmptyBook) {
    OrderBook book;
    EXPECT_FALSE(book.has_bids());
    EXPECT_FALSE(book.has_asks());
}

TEST(OrderBookTest, HasBidsTrueAfterAddBuy) {
    OrderBook book;
    book.add(Side::Buy, 1650000, 100);
    EXPECT_TRUE(book.has_bids());
    EXPECT_FALSE(book.has_asks());
}

TEST(OrderBookTest, HasAsksTrueAfterAddSell) {
    OrderBook book;
    book.add(Side::Sell, 1660000, 75);
    EXPECT_FALSE(book.has_bids());
    EXPECT_TRUE(book.has_asks());
}

TEST(OrderBookTest, ReAddAfterReduceToZero) {
    OrderBook book;
    book.add(Side::Buy, 1650000, 100);
    book.reduce(Side::Buy, 1650000, 100);
    ASSERT_FALSE(book.has_bids());

    book.add(Side::Buy,  1650000, 50);
    book.add(Side::Sell, 1660000, 30);

    auto bbo = book.get_bbo();
    ASSERT_TRUE(bbo.has_value());
    EXPECT_EQ(bbo->bid.price,  1650000);
    EXPECT_EQ(bbo->bid.shares, 50);
}

TEST(OrderBookTest, MixedAddReduceSequence) {
    OrderBook book;
    book.add(Side::Buy, 1650000, 100);
    book.reduce(Side::Buy, 1650000, 30);    // 70 left
    book.add(Side::Buy, 1650000, 20);       // 90 total
    book.reduce(Side::Buy, 1650000, 90);    // empty

    EXPECT_FALSE(book.has_bids());

    book.add(Side::Sell, 1660000, 50);
    EXPECT_TRUE(book.has_asks());
}

TEST(OrderBookTest, ReduceMoreThanLevelShares) {
    // Defensive: reducing 200 from a 100-share level should leave the side empty,
    // not underflow into a phantom level (Shares is unsigned).
    OrderBook book;
    book.add(Side::Buy, 1650000, 100);
    book.reduce(Side::Buy, 1650000, 200);

    EXPECT_FALSE(book.has_bids());
}

TEST(OrderBookTest, ReduceNonExistentLevelIsNoOp) {
    // Defensive: reducing on a price with no level should not create a phantom
    // level via operator[] semantics.
    OrderBook book;
    book.reduce(Side::Buy, 1650000, 50);

    EXPECT_FALSE(book.has_bids());
}
