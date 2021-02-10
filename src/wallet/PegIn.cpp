#include "PegIn.h"

#include <mw/crypto/Blinds.h>
#include <mw/wallet/Wallet.h>

mw::Transaction::CPtr PegIn::CreatePegInTx(const uint64_t amount, const StealthAddress& receiver_addr) const
{
    // Create peg-in output.
    // We randomly generate the sender_key and output_blind.
    // Receiver key is generated by Output::Create using the wallet's stealth address.
    SecretKey sender_key = Random::CSPRNG<32>();
    BlindingFactor output_blind;
    Output output = Output::Create(
        output_blind,
        EOutputFeatures::PEGGED_IN,
        sender_key,
        receiver_addr,
        amount
    );

    // Total kernel offset is split between raw kernel_offset and the kernel's blinding factor.
    // sum(output.blind) - sum(input.blind) = kernel_offset + sum(kernel.blind)
    BlindingFactor kernel_offset = Random::CSPRNG<32>();
    BlindingFactor kernel_blind = Blinds()
        .Add(output_blind)
        .Sub(kernel_offset)
        .Total();
    Kernel kernel = Kernel::CreatePegIn(kernel_blind, amount);

    // Total owner offset is split between raw owner_offset and the owner_sig's key.
    // sum(output.sender_key) - sum(input.key) = owner_offset + sum(owner_sig.key)
    BlindingFactor owner_sig_key = Random::CSPRNG<32>();
    SignedMessage owner_sig = Schnorr::SignMessage(owner_sig_key.GetBigInt(), kernel.GetHash());
    BlindingFactor owner_offset = Blinds()
        .Add(sender_key)
        .Sub(owner_sig_key)
        .Total();

    // If peg-in to user's own wallet, rewind output to retrieve and save the spendable coin.
    // This uses the same process that occurs on restore,
    // so a successful rewind means we know it can be restored from seed later.
    if (receiver_addr == m_wallet.GetPegInAddress()) { // TODO: Check all wallet addresses
        libmw::Coin coin = m_wallet.RewindOutput(output);
        m_wallet.GetInterface()->AddCoins({ std::move(coin) });
    }

    // Build the Transaction
    return mw::Transaction::Create(
        kernel_offset,
        owner_offset,
        std::vector<Input>{},
        std::vector<Output>{ std::move(output) },
        std::vector<Kernel>{ std::move(kernel) },
        std::vector<SignedMessage>{ std::move(owner_sig) }
    );
}