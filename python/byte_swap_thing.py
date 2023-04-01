

def byte_swap(storage, elem_size):
    storage_ = storage.copy()
    for i in range(int(len(storage_) / elem_size)):
        for k in range(int(elem_size / 2)):
            storage_[i * elem_size + k], storage_[i * elem_size + elem_size - k - 1] = storage_[i * elem_size + elem_size - k - 1], storage_[i * elem_size + k]

    return storage_

a = list(range(32))

print(byte_swap(a, 32))

