#include <atomic>
#include <stdint.h>
#include <string>

template <typename FloatType>
class RingBuffer {
public:
    RingBuffer (const uint64 numItems, const std::string& name) : name (name), buffer (numItems) {}

    uint64 numItemsAvailableForWrite() const {
        // signed space between readIndex and writeIndex index
        const long pointerSpace = readIndex.load() - writeIndex.load();
        // NB: > 0 so NOT including 0
        return pointerSpace > 0 ? pointerSpace : pointerSpace + size;
    }

    uint64 numItemsAvailableForRead() const {
        const auto pointerSpace = static_cast<int64_t> (writeIndex.load() - readIndex.load());
        return pointerSpace >= 0 ? pointerSpace : pointerSpace + size;
    }

    void pushMayBlock (bool block) {
        blockingPush = block;
    }

    void popMayBlock (bool block) {
        blockingPop = block;
    }
    
    void setBlockingNapMicroSeconds (const uint64 newBlockingNap) {
        blockingNap = newBlockingNap;
    }
    
    /// Try to write as many items as possible and return the number actually written
    uint64 push (FloatType* data, const uint64 numSamples) {
        auto space = size;

        if (blockingPush)
            while ((space = numItemsAvailableForWrite()) < numSamples)
                usleep (static_cast<useconds_t> (blockingNap));

        if (space == 0)
            return 0;

        const auto numToWrite = numSamples <= space ? numSamples : space;

        const auto currentWriteIndex = writeIndex.load();

        // wrap if needed
        if (currentWriteIndex + numToWrite <= size) {
            memcpy (buffer + currentWriteIndex, data, numToWrite * itemSize);
        } else {
            const auto firstChunk = size - currentWriteIndex;
            memcpy (buffer + currentWriteIndex, data, firstChunk * itemSize);
            memcpy (buffer, data + firstChunk, (numToWrite - firstChunk) * itemSize);
        }

        writeIndex.store ((currentWriteIndex + numToWrite) % size);

        return numToWrite;
    }

    /// Try to read as many items as possible and return the number actually read
    uint64 pop (FloatType* data, const uint64 numSamples)->uint64 {
        auto space = size;

        if (blockingPop)
            while ((space = numItemsAvailableForRead()) < numSamples)
                usleep (static_cast<useconds_t> (blockingNap));

        if (space == 0) return 0;

        const auto numToRead = numSamples <= space ? numSamples : space;

        const auto currentReadIndex = readIndex.load();

        // wrap if needed
        if (currentReadIndex + numToRead <= size) {
            memcpy (data, buffer + currentReadIndex, numToRead * itemSize);
        } else {
            const auto firstChunk = size - currentReadIndex;
            memcpy (data, buffer + currentReadIndex, firstChunk * itemSize);
            memcpy (data + firstChunk, buffer, (numToRead - firstChunk) * itemSize);
        }

        readIndex.store ((currentReadIndex + numToRead) % size);

        return numToRead;
    }

    bool isLockFree() const {
        return (writeIndex.is_lock_free() && readIndex.is_lock_free());
    }

private:
    const std::string name;
    std::vector<FloatType> buffer;

    std::atomic<uint64> writeIndex { 0 };
    std::atomic<uint64> readIndex { 0 };

    static constexpr uint64 itemSize { sizeof (FloatType) };

    bool blockingPush { false };
    bool blockingPop { false };

    uint64 blockingNap { 500 };
};
