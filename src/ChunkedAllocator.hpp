/*
Copyright (c) 2013 Benedikt Bitterli

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#ifndef CHUNKEDALLOCATOR_HPP_
#define CHUNKEDALLOCATOR_HPP_

#include <algorithm>
#include <cstring>
#include <vector>
#include <memory>

template<typename Type>
class ChunkedAllocator {
    static const size_t ChunkSize = 4096;

    struct InsertionPoint {
        size_t idx;
        Type data;
    };

    size_t _size;
    std::vector<std::unique_ptr<Type[]>> _data;
    std::vector<InsertionPoint> _insertions;

public:
    ChunkedAllocator() : _size(0) {}

    size_t size() {
        return _size;
    }

    size_t insertionCount() {
        return _insertions.size();
    }

    Type &operator[](size_t i) {
        return _data[i/ChunkSize][i % ChunkSize];
    }

    const Type &operator[](size_t i) const {
        return _data[i/ChunkSize][i % ChunkSize];
    }

    void pushBack(Type t) {
        if ((_size % ChunkSize) == 0)
            _data.emplace_back(new Type[ChunkSize]);

        _data[_size/ChunkSize][_size % ChunkSize] = std::move(t);
        _size++;
    }

    void insert(size_t index, Type data) {
        _insertions.emplace_back(InsertionPoint{index, std::move(data)});
    }

    std::unique_ptr<Type[]> finalize() {
        std::sort(_insertions.begin(), _insertions.end(), [](const InsertionPoint &a, const InsertionPoint &b) {
            return a.idx < b.idx;
        });

        size_t length = _size + _insertions.size();
        std::unique_ptr<Type[]> result(new Type[length]);

        size_t insertionIdx = 0;
        size_t outputOffset = 0;
        size_t inputOffset = 0;
        while (inputOffset < _size) {
            size_t dataIdx    = inputOffset/ChunkSize;
            size_t dataOffset = inputOffset % ChunkSize;

            // Clear out data blocks once we've copied them completely
            if (dataOffset == 0 && dataIdx > 0)
                _data[dataIdx - 1].reset();

            size_t copySize = std::min(ChunkSize - dataOffset, _size - inputOffset);
            if (insertionIdx < _insertions.size())
                copySize = std::min(copySize, _insertions[insertionIdx].idx - inputOffset);

            if (copySize > 0) {
                std::memcpy(result.get() + outputOffset, _data[dataIdx].get() + dataOffset, copySize*sizeof(Type));
                inputOffset += copySize;
                outputOffset += copySize;
            }

            if (insertionIdx < _insertions.size() && _insertions[insertionIdx].idx == inputOffset)
                result[outputOffset++] = std::move(_insertions[insertionIdx++].data);
        }

        _insertions.clear();
        _data.clear();
        _size = 0;

        return std::move(result);
    }
};

#endif /* CHUNKEDALLOCATOR_HPP_ */
