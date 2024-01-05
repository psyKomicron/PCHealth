namespace Common
{
    class FileSize
    {
    public:
        FileSize() = default;
        FileSize(const uint64_t& size);

        uint64_t Size() const;
        std::wstring ToString() const;

        FileSize operator=(const uint64_t& value);

    private:
        uint64_t size;
    };
}
