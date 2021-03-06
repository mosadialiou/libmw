#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Macros.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/crypto/Hash.h>
#include <mw/traits/Hashable.h>
#include <mw/traits/Serializable.h>
#include <mw/traits/Printable.h>
#include <mw/traits/Jsonable.h>
#include <mw/serialization/Serializer.h>
#include <mw/crypto/Hasher.h>

#include <boost/optional.hpp>
#include <cstdint>
#include <memory>

MW_NAMESPACE

class Header final :
    public Traits::IPrintable,
    public Traits::ISerializable,
    public Traits::IHashable,
    public Traits::IJsonable
{
public:
    using CPtr = std::shared_ptr<const Header>;

    //
    // Constructors
    //
    Header() = default;
    Header(
        const uint64_t height,
        mw::Hash&& outputRoot,
        mw::Hash&& rangeProofRoot,
        mw::Hash&& kernelRoot,
		mw::Hash&& leafsetRoot,
        BlindingFactor&& offset,
        const uint64_t outputMMRSize,
        const uint64_t kernelMMRSize
    )
        : m_height(height),
        m_outputRoot(std::move(outputRoot)),
        m_rangeProofRoot(std::move(rangeProofRoot)),
        m_kernelRoot(std::move(kernelRoot)),
		m_leafsetRoot(std::move(leafsetRoot)),
        m_offset(std::move(offset)),
        m_outputMMRSize(outputMMRSize),
        m_kernelMMRSize(kernelMMRSize) { }

    //
    // Operators
    //
    bool operator!=(const Header& rhs) const noexcept { return this->GetHash() != rhs.GetHash(); }

    //
    // Getters
    //
    uint64_t GetHeight() const noexcept { return m_height; }
    const mw::Hash& GetOutputRoot() const noexcept { return m_outputRoot; }
    const mw::Hash& GetRangeProofRoot() const noexcept { return m_rangeProofRoot; }
    const mw::Hash& GetKernelRoot() const noexcept { return m_kernelRoot; }
	const mw::Hash& GetLeafsetRoot() const noexcept { return m_leafsetRoot; }
    const BlindingFactor& GetOffset() const noexcept { return m_offset; }
    uint64_t GetNumTXOs() const noexcept { return m_outputMMRSize; }
    uint64_t GetNumKernels() const noexcept { return m_kernelMMRSize; }

    //
    // Traits
    //
    mw::Hash GetHash() const noexcept final
    {
        if (!m_hash.has_value())
        {
            m_hash = boost::make_optional(Hashed(*this));
        }

        return m_hash.value();
    }

    std::string Format() const final { return GetHash().ToHex(); }

    //
    // Serialization/Deserialization
    //
    Serializer& Serialize(Serializer& serializer) const noexcept final
    {
        return serializer
            .Append<uint64_t>(m_height)
            .Append(m_outputRoot)
            .Append(m_rangeProofRoot)
            .Append(m_kernelRoot)
			.Append(m_leafsetRoot)
            .Append(m_offset)
            .Append<uint64_t>(m_outputMMRSize)
            .Append<uint64_t>(m_kernelMMRSize);
    }

    static Header Deserialize(Deserializer& deserializer)
    {
        uint64_t height = deserializer.Read<uint64_t>();
        mw::Hash outputRoot = mw::Hash::Deserialize(deserializer);
        mw::Hash proofRoot = mw::Hash::Deserialize(deserializer);
		mw::Hash kernelRoot = mw::Hash::Deserialize(deserializer);
		mw::Hash leafsetRoot = mw::Hash::Deserialize(deserializer);
        BlindingFactor offset = BlindingFactor::Deserialize(deserializer);
        uint64_t outputMMRSize = deserializer.Read<uint64_t>();
        uint64_t kernelMMRSize = deserializer.Read<uint64_t>();

        return Header{
            height,
            std::move(outputRoot),
            std::move(proofRoot),
            std::move(kernelRoot),
			std::move(leafsetRoot),
            std::move(offset),
            outputMMRSize,
            kernelMMRSize
        };
    }

    json ToJSON() const noexcept final
    {
        // TODO: Implement
        return json();
    }

    static Header FromJSON(const Json& json)
    {
        // TODO: Implement
        throw std::exception();
    }

    //
    // Context-free validation of the header.
    //
    void Validate() const
    {
        // TODO: There may not be anything to validate
    }

private:
    mutable boost::optional<mw::Hash> m_hash;
    uint64_t m_height;
    mw::Hash m_outputRoot;
    mw::Hash m_rangeProofRoot;
    mw::Hash m_kernelRoot;
	mw::Hash m_leafsetRoot;
    BlindingFactor m_offset;
    uint64_t m_outputMMRSize;
    uint64_t m_kernelMMRSize;
};

END_NAMESPACE