#include <libmw/libmw.h>

#include "Transformers.h"
#include "BlockStoreWrapper.h"
#include "State.h"

#include <mw/models/block/Block.h>
#include <mw/models/block/BlockUndo.h>
#include <mw/models/tx/Transaction.h>
#include <mw/models/tx/UTXO.h>
#include <mw/node/INode.h>
#include <mw/wallet/Wallet.h>

static mw::INode::Ptr NODE = nullptr;

LIBMW_NAMESPACE

EXPORT libmw::HeaderRef DeserializeHeader(const std::vector<uint8_t>& bytes)
{
    Deserializer deserializer{ bytes };
    auto pHeader = std::make_shared<mw::Header>(mw::Header::Deserialize(deserializer));
    return libmw::HeaderRef{ pHeader };
}

EXPORT std::vector<uint8_t> SerializeHeader(const libmw::HeaderRef& header)
{
    assert(header.pHeader != nullptr);
    return header.pHeader->Serialized();
}

EXPORT libmw::BlockRef DeserializeBlock(const std::vector<uint8_t>& bytes)
{
    Deserializer deserializer{ bytes };
    auto pBlock = std::make_shared<mw::Block>(mw::Block::Deserialize(deserializer));
    return libmw::BlockRef{ pBlock };
}

EXPORT std::vector<uint8_t> SerializeBlock(const libmw::BlockRef& block)
{
    assert(block.pBlock != nullptr);
    return block.pBlock->Serialized();
}

EXPORT libmw::HeaderRef BlockRef::GetHeader() const
{
    if (pBlock == nullptr) {
        return libmw::HeaderRef{ nullptr };
    }

    return libmw::HeaderRef{ pBlock->GetHeader() };
}

EXPORT libmw::BlockUndoRef DeserializeBlockUndo(const std::vector<uint8_t>& bytes)
{
    LOG_TRACE("Deserializing BlockUndo");

    Deserializer deserializer{ bytes };
    auto pBlockUndo = std::make_shared<mw::BlockUndo>(mw::BlockUndo::Deserialize(deserializer));
    return libmw::BlockUndoRef{ pBlockUndo };
}

EXPORT std::vector<uint8_t> SerializeBlockUndo(const libmw::BlockUndoRef& blockUndo)
{
    assert(blockUndo.pUndo != nullptr);
    LOG_TRACE_F("Serializing BlockUndo for {}", blockUndo.pUndo->GetPreviousHeader());
    return blockUndo.pUndo->Serialized();
}

EXPORT std::vector<PegOut> TxRef::GetPegouts() const noexcept
{
    std::vector<PegOut> pegouts;
    for (const Kernel& kernel : pTransaction->GetKernels()) {
        if (kernel.IsPegOut()) {
            PegOut pegout;
            pegout.amount = kernel.GetPeggedOut();
            pegout.address = kernel.GetAddress().value().ToString();
            pegouts.emplace_back(std::move(pegout));
        }
    }

    return pegouts;
}

EXPORT uint64_t TxRef::GetTotalFee() const noexcept
{
    return pTransaction->GetTotalFee();
}

EXPORT libmw::TxRef DeserializeTx(const std::vector<uint8_t>& bytes)
{
    LOG_TRACE("Deserializing tx");
    Deserializer deserializer{ bytes };
    auto pTx = std::make_shared<mw::Transaction>(mw::Transaction::Deserialize(deserializer));
    return libmw::TxRef{ pTx };
}

EXPORT std::vector<uint8_t> SerializeTx(const libmw::TxRef& tx)
{
    return tx.pTransaction->Serialized();
}

EXPORT libmw::CoinsViewRef CoinsViewRef::CreateCache() const
{
    if (pCoinsView == nullptr) {
        return libmw::CoinsViewRef{ nullptr };
    }

    return libmw::CoinsViewRef{ std::make_shared<mw::CoinsViewCache>(pCoinsView) };
}

NODE_NAMESPACE

EXPORT libmw::CoinsViewRef Initialize(
    const libmw::ChainParams& chainParams,
    const libmw::HeaderRef& header,
    const std::shared_ptr<libmw::IDBWrapper>& pDBWrapper)
{
    NODE = mw::InitializeNode(FilePath{ chainParams.dataDirectory }, chainParams.hrp, header.pHeader, pDBWrapper);

    return libmw::CoinsViewRef{ NODE->GetDBView() };
}

EXPORT libmw::CoinsViewRef ApplyState(
    const libmw::IBlockStore::Ptr& pBlockStore,
    const libmw::IDBWrapper::Ptr& pCoinsDB,
    const libmw::BlockHash& firstMWHeaderHash,
    const libmw::BlockHash& stateHeaderHash,
    const libmw::StateRef& state)
{
    BlockStoreWrapper blockStore(pBlockStore.get());
    auto pCoinsViewDB = NODE->ApplyState(
        pCoinsDB,
        blockStore,
        mw::Hash{ firstMWHeaderHash },
        mw::Hash{ stateHeaderHash },
        state.pState->utxos,
        state.pState->kernels
    );

    return libmw::CoinsViewRef{ pCoinsViewDB };
}

EXPORT void CheckBlock(const libmw::BlockRef& block, const std::vector<libmw::PegIn>& pegInCoins, const std::vector<libmw::PegOut>& pegOutCoins)
{
    auto pegins = TransformPegIns(pegInCoins);
    auto pegouts = TransformPegOuts(pegOutCoins);
    NODE->ValidateBlock(block.pBlock, pegins, pegouts);
}

EXPORT libmw::BlockUndoRef ConnectBlock(const libmw::BlockRef& block, const CoinsViewRef& view)
{
    return libmw::BlockUndoRef{ NODE->ConnectBlock(block.pBlock, view.pCoinsView) };
}

EXPORT void DisconnectBlock(const libmw::BlockUndoRef& undoData, const CoinsViewRef& view)
{
    NODE->DisconnectBlock(undoData.pUndo, view.pCoinsView);
}

EXPORT libmw::BlockRef BuildNextBlock(
    const uint64_t height,
    const libmw::CoinsViewRef& view,
    const std::vector<libmw::TxRef>& transactions,
    const std::vector<libmw::PegIn>& pegInCoins,
    const std::vector<libmw::PegOut>& pegOutCoins)
{
    mw::CoinsViewCache viewCache(view.pCoinsView);
    //auto pViewCache = dynamic_cast<mw::CoinsViewCache*>(view.pCoinsView.get());
    //assert(pViewCache != nullptr);

    LOG_TRACE_F("Building block with {} txs", transactions.size());
    auto txs = TransformTxs(transactions);
    auto pBlock = viewCache.BuildNextBlock(height, txs);
    LOG_DEBUG_F("Next block built: {}", Json(pBlock->ToJSON()));

    return libmw::BlockRef{ pBlock };
}

EXPORT void FlushCache(const libmw::CoinsViewRef& view, const std::unique_ptr<libmw::IDBBatch>& pBatch)
{
    LOG_TRACE("Flushing cache");
    auto pViewCache = dynamic_cast<mw::CoinsViewCache*>(view.pCoinsView.get());
    assert(pViewCache != nullptr);

    pViewCache->Flush(pBatch);
    LOG_TRACE("Cache flushed");
}

EXPORT libmw::StateRef DeserializeState(const std::vector<uint8_t>& bytes)
{
    Deserializer deserializer{ bytes };
    mw::State state = mw::State::Deserialize(deserializer);
    return { std::make_shared<mw::State>(std::move(state)) };
}

EXPORT std::vector<uint8_t> SerializeState(const libmw::StateRef& state)
{
    return state.pState->Serialized();
}

EXPORT libmw::StateRef SnapshotState(const libmw::CoinsViewRef& view, const libmw::BlockHash& block_hash)
{
    return { nullptr }; // TODO: Implement
}

EXPORT void CheckTransaction(const libmw::TxRef& transaction)
{
    // TODO: Implement
}

END_NAMESPACE // node

WALLET_NAMESPACE

EXPORT std::pair<libmw::TxRef, libmw::PegIn> CreatePegInTx(const libmw::IWallet::Ptr& pWallet, const uint64_t amount)
{
    mw::Transaction::CPtr pTx = Wallet(pWallet).CreatePegInTx(amount);

    assert(!pTx->GetKernels().empty());
    libmw::Commitment commit = pTx->GetKernels().front().GetCommitment().array();
    return std::make_pair(libmw::TxRef{ pTx }, libmw::PegIn{ amount, commit });
}

EXPORT std::pair<libmw::TxRef, libmw::PegOut> CreatePegOutTx(
    const libmw::IWallet::Ptr& pWallet,
    const uint64_t amount,
    const std::string& address)
{
    mw::Transaction::CPtr pTx = Wallet(pWallet).CreatePegOutTx(amount, Bech32Address::FromString(address));
    return std::make_pair(libmw::TxRef{ pTx }, libmw::PegOut{ amount, address });
}

END_NAMESPACE // wallet
END_NAMESPACE // libmw