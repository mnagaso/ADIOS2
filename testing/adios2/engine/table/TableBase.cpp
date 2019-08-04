/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>
#include <gtest/gtest.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif
#include <numeric>
#include <thread>

using namespace adios2;
int mpiRank = 0;
int mpiSize = 1;
size_t print_lines = 0;

char runMode;

class TableEngineTest : public ::testing::Test
{
public:
    TableEngineTest() = default;
};

template <class T>
void PrintData(const T *data, const size_t step, const Dims &start,
               const Dims &count)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    std::cout << "Rank: " << mpiRank << " Step: " << step << " Size:" << size
              << "\n";
    size_t printsize = 128;

    if (size < printsize)
    {
        printsize = size;
    }
    int s = 0;
    for (size_t i = 0; i < printsize; ++i)
    {
        ++s;
        std::cout << data[i] << " ";
        if (s == count[1])
        {
            std::cout << std::endl;
            s = 0;
        }
    }

    std::cout << "]" << std::endl;
}

template <class T>
void GenData(std::vector<T> &data, const size_t step, const Dims &start, const Dims &count, const Dims &shape)
{
    for (size_t i = 0; i < count[1]; ++i)
    {
        for (size_t j = 0; j < count[2]; ++j)
        {
            data[i * count[2] + j] = i * count[2] + j;
        }
    }
}

template <class T>
void VerifyData(const std::complex<T> *data, size_t step, const Dims &start,
                const Dims &count, const Dims &shape)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    std::vector<std::complex<T>> tmpdata(size);
    GenData(tmpdata, step, start, count, shape);
    for (size_t i = 0; i < size; ++i)
    {
        ASSERT_EQ(data[i], tmpdata[i]);
    }
    if (print_lines < 100)
    {
        PrintData(data, step, start, count);
        ++print_lines;
    }
}

template <class T>
void VerifyData(const T *data, size_t step, const Dims &start,
                const Dims &count, const Dims &shape)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    bool compressed = false;
    std::vector<T> tmpdata(size);
    if (print_lines < 100)
    {
        PrintData(data, step, start, count);
        ++print_lines;
    }
    GenData(tmpdata, step, start, count, shape);
    for (size_t i = 0; i < size; ++i)
    {
        if (!compressed)
        {
            ASSERT_EQ(data[i], tmpdata[i]);
        }
    }
}

void Reader(const Dims &shape, const Dims &start, const Dims &count,
            const size_t steps, const adios2::Params &engineParams,
            const std::string &name)
{
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("Test");
    dataManIO.SetEngine("BPFile");
    dataManIO.SetParameters(engineParams);
    adios2::Engine dataManReader = dataManIO.Open(name, adios2::Mode::Read);
    size_t datasize = std::accumulate(count.begin(), count.end(), 1, std::multiplies<size_t>());
    std::vector<char> myChars(datasize);
    std::vector<unsigned char> myUChars(datasize);
    std::vector<short> myShorts(datasize);
    std::vector<unsigned short> myUShorts(datasize);
    std::vector<int> myInts(datasize);
    std::vector<unsigned int> myUInts(datasize);
    std::vector<float> myFloats(datasize);
    std::vector<double> myDoubles(datasize);
    std::vector<std::complex<float>> myComplexes(datasize);
    std::vector<std::complex<double>> myDComplexes(datasize);

    bool received_steps = false;
    size_t i;
    for (i = 0; i < steps; ++i)
    {

        adios2::StepStatus status = dataManReader.BeginStep(StepMode::Read, 5);

        received_steps = true;
        const auto &vars = dataManIO.AvailableVariables();
        if (print_lines == 0)
        {
            std::cout << "All available variables : ";
            for (const auto &var : vars)
            {
                std::cout << var.first << ", ";
            }
            std::cout << std::endl;
        }
        ASSERT_EQ(vars.size(), 10);
        size_t currentStep = dataManReader.CurrentStep();
        adios2::Variable<char> bpChars =
            dataManIO.InquireVariable<char>("bpChars");
        adios2::Variable<unsigned char> bpUChars =
            dataManIO.InquireVariable<unsigned char>("bpUChars");
        adios2::Variable<short> bpShorts =
            dataManIO.InquireVariable<short>("bpShorts");
        adios2::Variable<unsigned short> bpUShorts =
            dataManIO.InquireVariable<unsigned short>("bpUShorts");
        adios2::Variable<int> bpInts =
            dataManIO.InquireVariable<int>("bpInts");
        adios2::Variable<unsigned int> bpUInts =
            dataManIO.InquireVariable<unsigned int>("bpUInts");
        adios2::Variable<float> bpFloats =
            dataManIO.InquireVariable<float>("bpFloats");
        adios2::Variable<double> bpDoubles =
            dataManIO.InquireVariable<double>("bpDoubles");
        adios2::Variable<std::complex<float>> bpComplexes =
            dataManIO.InquireVariable<std::complex<float>>("bpComplexes");
        adios2::Variable<std::complex<double>> bpDComplexes =
            dataManIO.InquireVariable<std::complex<double>>("bpDComplexes");
        auto charsBlocksInfo = dataManReader.AllStepsBlocksInfo(bpChars);

        bpChars.SetSelection({start, count});
        bpUChars.SetSelection({start, count});
        bpShorts.SetSelection({start, count});
        bpUShorts.SetSelection({start, count});
        bpInts.SetSelection({start, count});
        bpUInts.SetSelection({start, count});
        bpFloats.SetSelection({start, count});
        bpDoubles.SetSelection({start, count});
        bpComplexes.SetSelection({start, count});
        bpDComplexes.SetSelection({start, count});

        dataManReader.Get(bpChars, myChars.data(), adios2::Mode::Sync);
        dataManReader.Get(bpUChars, myUChars.data(), adios2::Mode::Sync);
        dataManReader.Get(bpShorts, myShorts.data(), adios2::Mode::Sync);
        dataManReader.Get(bpUShorts, myUShorts.data(), adios2::Mode::Sync);
        dataManReader.Get(bpInts, myInts.data(), adios2::Mode::Sync);
        dataManReader.Get(bpUInts, myUInts.data(), adios2::Mode::Sync);
        dataManReader.Get(bpFloats, myFloats.data(), adios2::Mode::Sync);
        dataManReader.Get(bpDoubles, myDoubles.data(), adios2::Mode::Sync);
        dataManReader.Get(bpComplexes, myComplexes.data(), adios2::Mode::Sync);
        dataManReader.Get(bpDComplexes, myDComplexes.data(), adios2::Mode::Sync);
        VerifyData(myChars.data(), currentStep, start, count, shape);
        VerifyData(myUChars.data(), currentStep, start, count, shape);
        VerifyData(myShorts.data(), currentStep, start, count, shape);
        VerifyData(myUShorts.data(), currentStep, start, count, shape);
        VerifyData(myInts.data(), currentStep, start, count, shape);
        VerifyData(myUInts.data(), currentStep, start, count, shape);
        VerifyData(myFloats.data(), currentStep, start, count, shape);
        VerifyData(myDoubles.data(), currentStep, start, count, shape);
        VerifyData(myComplexes.data(), currentStep, start, count, shape);
        VerifyData(myDComplexes.data(), currentStep, start, count, shape);
        dataManReader.EndStep();
    }
    dataManReader.Close();
    print_lines = 0;
}

void Writer(const Dims &shape, const Dims &start, const Dims &count, const size_t rows, const adios2::Params &engineParams, const std::string &name)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1, std::multiplies<size_t>());
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("ms");
    dataManIO.SetEngine("table");
    dataManIO.SetParameters(engineParams);
    std::vector<char> myChars(datasize);
    std::vector<unsigned char> myUChars(datasize);
    std::vector<short> myShorts(datasize);
    std::vector<unsigned short> myUShorts(datasize);
    std::vector<int> myInts(datasize);
    std::vector<unsigned int> myUInts(datasize);
    std::vector<float> myFloats(datasize);
    std::vector<double> myDoubles(datasize);
    std::vector<std::complex<float>> myComplexes(datasize);
    std::vector<std::complex<double>> myDComplexes(datasize);
    auto bpChars = dataManIO.DefineVariable<char>("bpChars", shape, start, count);
    auto bpUChars = dataManIO.DefineVariable<unsigned char>("bpUChars", shape, start, count);
    auto bpShorts = dataManIO.DefineVariable<short>("bpShorts", shape, start, count);
    auto bpUShorts = dataManIO.DefineVariable<unsigned short>( "bpUShorts", shape, start, count);
    auto bpInts = dataManIO.DefineVariable<int>("bpInts", shape, start, count);
    auto bpUInts = dataManIO.DefineVariable<unsigned int>("bpUInts", shape, start, count);
    auto bpFloats = dataManIO.DefineVariable<float>("bpFloats", shape, start, count);
    auto bpDoubles = dataManIO.DefineVariable<double>("bpDoubles", shape, start, count);
    auto bpComplexes = dataManIO.DefineVariable<std::complex<float>>( "bpComplexes", shape, start, count);
    auto bpDComplexes = dataManIO.DefineVariable<std::complex<double>>( "bpDComplexes", shape, start, count);
    adios2::Engine tableWriter = dataManIO.Open(name, adios2::Mode::Write);
    tableWriter.BeginStep();
    for (int i = 0; i < rows; ++i)
    {
        Dims startRow = start;
        startRow[0] = i;
        bpChars.SetSelection({startRow, count});
        bpUChars.SetSelection({startRow, count});
        bpShorts.SetSelection({startRow, count});
        bpUShorts.SetSelection({startRow, count});
        bpInts.SetSelection({startRow, count});
        bpUInts.SetSelection({startRow, count});
        bpFloats.SetSelection({startRow, count});
        bpDoubles.SetSelection({startRow, count});
        bpComplexes.SetSelection({startRow, count});
        bpDComplexes.SetSelection({startRow, count});
        GenData(myChars, i, startRow, count, shape);
        GenData(myUChars, i, startRow, count, shape);
        GenData(myShorts, i, startRow, count, shape);
        GenData(myUShorts, i, startRow, count, shape);
        GenData(myInts, i, startRow, count, shape);
        GenData(myUInts, i, startRow, count, shape);
        GenData(myFloats, i, startRow, count, shape);
        GenData(myDoubles, i, startRow, count, shape);
        GenData(myComplexes, i, startRow, count, shape);
        GenData(myDComplexes, i, startRow, count, shape);
        tableWriter.Put(bpChars, myChars.data(), adios2::Mode::Sync);
        tableWriter.Put(bpUChars, myUChars.data(), adios2::Mode::Sync);
        tableWriter.Put(bpShorts, myShorts.data(), adios2::Mode::Sync);
        tableWriter.Put(bpUShorts, myUShorts.data(), adios2::Mode::Sync);
        tableWriter.Put(bpInts, myInts.data(), adios2::Mode::Sync);
        tableWriter.Put(bpUInts, myUInts.data(), adios2::Mode::Sync);
        tableWriter.Put(bpFloats, myFloats.data(), adios2::Mode::Sync);
        tableWriter.Put(bpDoubles, myDoubles.data(), adios2::Mode::Sync);
        tableWriter.Put(bpComplexes, myComplexes.data(), adios2::Mode::Sync);
        tableWriter.Put(bpDComplexes, myDComplexes.data(), adios2::Mode::Sync);
    }
    tableWriter.EndStep();
    tableWriter.Close();
}

TEST_F(TableEngineTest, TableBase)
{
    std::string filename = "TableBase";
    adios2::Params engineParams = {{"Verbose", "11"}};

    size_t rows = 1000;
    Dims shape = {rows, 1, 128};
    Dims start = {0, 0, 0};
    Dims count = {1, 1, 128};

    Writer(shape, start, count, rows, engineParams, filename);

//        Reader(shape, start, count, 10, engineParams, filename);

    MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();
    return result;
}
