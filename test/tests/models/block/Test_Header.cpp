#include <catch.hpp>

#include <mw/models/block/Header.h>

TEST_CASE("Header")
{
    const uint64_t height = 1;
    const uint64_t outputMMRSize = 2;
    const uint64_t kernelMMRSize = 3;

    mw::Hash outputRoot = mw::Hash::FromHex("000102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");
    mw::Hash rangeProofRoot = mw::Hash::FromHex("001102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");
    mw::Hash kernelRoot = mw::Hash::FromHex("002102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");
    mw::Hash leafsetRoot = mw::Hash::FromHex("003102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");
    BlindingFactor offset = BigInt<32>::FromHex("004102030405060708090A0B0C0D0E0F1112131415161718191A1B1C1D1E1F20");

    mw::Header header(
        height,
        mw::Hash(outputRoot),
        mw::Hash(rangeProofRoot),
        mw::Hash(kernelRoot),
        mw::Hash(leafsetRoot),
        BlindingFactor(offset),
        outputMMRSize,
        kernelMMRSize
    );
    mw::Header header2(
        height + 1,
        mw::Hash(outputRoot),
        mw::Hash(rangeProofRoot),
        mw::Hash(kernelRoot),
        mw::Hash(leafsetRoot),
        BlindingFactor(offset),
        outputMMRSize,
        kernelMMRSize
    );

    REQUIRE_FALSE(header == header2);
    REQUIRE(header.GetHeight() == height);
    REQUIRE(header.GetOutputRoot() == outputRoot);
    REQUIRE(header.GetRangeProofRoot() == rangeProofRoot);
    REQUIRE(header.GetKernelRoot() == kernelRoot);
    REQUIRE(header.GetLeafsetRoot() == leafsetRoot);
    REQUIRE(header.GetOffset() == offset);
    REQUIRE(header.GetNumTXOs() == outputMMRSize);
    REQUIRE(header.GetNumKernels() == kernelMMRSize);
    REQUIRE(header.Format() == "5cce9d8b13823e676e3d2e4a08a261f1f000c1bf9a29143b96f955f2ab6d4539");

    Deserializer deserializer = header.Serialized();
    REQUIRE(header == mw::Header::Deserialize(deserializer));
    REQUIRE(header == mw::Header::FromJSON(header.ToJSON()));
}