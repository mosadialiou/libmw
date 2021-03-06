#pragma once

#include <mw/common/Macros.h>
#include <mw/models/tx/Output.h>
#include <mw/mmr/LeafIndex.h>
#include <mw/traits/Serializable.h>
#include <mw/serialization/Serializer.h>

class UTXO : public Traits::ISerializable
{
public:
    using CPtr = std::shared_ptr<const UTXO>;

    UTXO() = default;
    UTXO(const uint64_t blockHeight, mmr::LeafIndex&& leafIdx, Output&& output)
        : m_blockHeight(blockHeight), m_leafIdx(std::move(leafIdx)), m_output(std::move(output)) { }
    UTXO(const uint64_t blockHeight, mmr::LeafIndex&& leafIdx, const Output& output)
        : m_blockHeight(blockHeight), m_leafIdx(std::move(leafIdx)), m_output(output) { }

    uint64_t GetBlockHeight() const noexcept { return m_blockHeight; }
    const mmr::LeafIndex GetLeafIndex() const noexcept { return m_leafIdx; }
    const Output& GetOutput() const noexcept { return m_output; }

	const Commitment& GetCommitment() const noexcept { return m_output.GetCommitment(); }
    const std::vector<uint8_t>& GetExtraData() const noexcept { return m_output.GetExtraData(); }
	RangeProof::CPtr GetRangeProof() const noexcept { return m_output.GetRangeProof(); }

    Serializer& Serialize(Serializer& serializer) const noexcept
    {
        return serializer
            .Append<uint64_t>(m_blockHeight)
            .Append<uint64_t>(m_leafIdx.GetLeafIndex())
            .Append(m_output);
    }

    static UTXO Deserialize(Deserializer& deserializer)
    {
        const uint64_t blockHeight = deserializer.Read<uint64_t>();
        const uint64_t leafIdx = deserializer.Read<uint64_t>();
        Output output = Output::Deserialize(deserializer);
        return UTXO(blockHeight, mmr::LeafIndex::At(leafIdx), std::move(output));
    }

private:
    uint64_t m_blockHeight;
    mmr::LeafIndex m_leafIdx;
    Output m_output;
};