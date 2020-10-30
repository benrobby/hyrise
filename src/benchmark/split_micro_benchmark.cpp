#include <fstream>

#include "micro_benchmark_basic_fixture.hpp"

#include "benchmark_config.hpp"
#include "expression/expression_functional.hpp"
#include "hyrise.hpp"
#include "operators/index_scan.hpp"
#include "operators/table_scan.hpp"
#include "operators/table_wrapper.hpp"
#include "storage/chunk_encoder.hpp"
#include "storage/encoding_type.hpp"
#include "storage/index/group_key/group_key_index.hpp"

#include "utils/load_table.hpp"
#include "operators/print.hpp"
#include "operators/sort.hpp"

using namespace opossum::expression_functional;  // NOLINT

namespace {
using namespace opossum;

///////////////////////////////
// benchmark seetings
///////////////////////////////

// input and output settings 
///////////////////////////////
constexpr auto SEGMENT_META_DATA_FILE = "../../out/timestamp/split_segment_meta_data_int_index.csv";
constexpr auto INDEX_META_DATA_FILE = "../../out/timestamp/split_index_meta_data_int_index.csv";
constexpr auto TBL_FILE = "../../data/timestamps.tbl";

// table and compression settings
///////////////////////////////
constexpr auto TABLE_NAME_PREFIX = "timestamp";
const auto CHUNK_SIZE = size_t{10'000'000};
const auto SCAN_COLUMNS = std::vector{"YEAR", "MONTH", "DAY", "HOUR", "MINUTE", "SECOND"};
// Frame of References supports only int columns
// Dictionary Encoding should always have the id 0
const auto CHUNK_ENCODINGS = std::vector{SegmentEncodingSpec{EncodingType::Dictionary}, SegmentEncodingSpec{EncodingType::Unencoded}, SegmentEncodingSpec{EncodingType::LZ4}, SegmentEncodingSpec{EncodingType::RunLength}};
//const auto CHUNK_ENCODINGS = std::vector{SegmentEncodingSpec{EncodingType::Dictionary}, SegmentEncodingSpec{EncodingType::Unencoded}, SegmentEncodingSpec{EncodingType::LZ4}, SegmentEncodingSpec{EncodingType::RunLength}};
//const auto CHUNK_ENCODINGS = std::vector{SegmentEncodingSpec{EncodingType::Dictionary}};
const auto CREATE_INDEX = false; 

// quantile benchmark values (mixed data type table)
// determined by column stats python script calculated with pandas, settings nearest 
///////////////////////////////
// const auto BM_VAL_CAPTAIN_ID = std::vector{4, 4115, 11787, 57069, 176022, 451746, 616628, 901080, 1156169, 1233112, 1414788};
// const auto BM_VAL_CAPTAIN_STATUS = std::vector{1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2};
// const auto BM_VAL_LATITUDE = std::vector{10.2191832, 25.0455204, 25.0699667, 25.0872227, 25.1030861, 25.1244186, 25.1724729, 25.1966912, 25.2164364, 25.2437205, 60.1671321};
// const auto BM_VAL_LONGITUDE = std::vector{-213.5243575, 55.1423584, 55.1549474, 55.1792718, 55.2072508, 55.2470842, 55.2692599, 55.2806365, 55.3156638, 55.3640991, 212.33914};
// const auto BM_VAL_TIMESTAMP = std::vector{"2018-11-05 00:01:19", "2018-11-05 07:17:17", "2018-11-05 12:49:23", "2018-11-05 18:38:21", "2018-11-22 23:04:04", "2018-12-22 00:20:20", "2018-12-22 11:34:55", "2018-12-22 23:33:37", "2019-01-28 04:06:07", "2019-01-28 11:35:48", "2019-01-29 00:01:02"};

// 10 mio pings sort table

// quantile benchmark values (int table)
// timestamp values --> unix timestamp
// [0.0001, 0.01, 0.02, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.9, 1.0]
///////////////////////////////
const auto BM_VAL_YEAR = std::vector{2018, 2018, 2018};
const auto BM_VAL_MONTH = std::vector{11, 11, 11};
const auto BM_VAL_DAY = std::vector{1, 2, 3};
const auto BM_VAL_HOUR = std::vector{1, 6, 12};
const auto BM_VAL_MINUTE = std::vector{1, 15, 30};
const auto BM_VAL_SECOND = std::vector{1, 15, 30};

const auto BM_SCAN_VALUES = BM_VAL_YEAR.size();

// quantile between benchmark values (int table)
// timestamp values --> unix timestamp
// [0.0001, 0.01, 0.02, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.9, 1.0]
///////////////////////////////
const std::vector<std::vector<int>> BM_BETWEEN_VAL_YEAR {{2018, 2018}, {2018, 2018}, {2018, 2018}};
const std::vector<std::vector<int>> BM_BETWEEN_VAL_MONTH {{11, 11}, {11, 11}, {11, 11}};
const std::vector<std::vector<int>> BM_BETWEEN_VAL_DAY {{1, 2}, {2, 2}, {1, 3}};
const std::vector<std::vector<int>> BM_BETWEEN_VAL_HOUR {{11, 13}, {9, 15}, {4, 20}};
const std::vector<std::vector<int>> BM_BETWEEN_VAL_MINUTE {{29, 31}, {20, 40}, {10, 50}};
const std::vector<std::vector<int>> BM_BETWEEN_VAL_SECOND {{29, 31}, {20, 40}, {10, 50}};

// 10 mio pings table

// quantile benchmark values (int table)
// timestamp values --> unix timestamp
// [0.0001, 0.01, 0.02, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.9, 1.0]
///////////////////////////////
//const auto BM_VAL_CAPTAIN_ID = std::vector{4, 464, 844, 1628, 4115, 6362, 11787, 24882, 57069, 176022, 451746, 616628, 901080, 954443, 1156169, 1233112, 1414788};
//const auto BM_VAL_CAPTAIN_STATUS = std::vector{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2};
//const auto BM_VAL_LATITUDE = std::vector{243973175, 249279532, 249973158, 250299799, 250455204, 250573540, 250699666, 250775666, 250872227, 251030861, 251244185, 251724729, 251966912, 252081200, 252164364, 252437204, 601671321};
//const auto BM_VAL_LONGITUDE = std::vector{543540652, 550593981, 551164532, 551349072, 551423584, 551481989, 551549474, 551685925, 551792718, 552072508, 552470842, 552692599, 552806365, 552898481, 553156638, 553640991, 2123391399};
//const auto BM_VAL_TIMESTAMP = std::vector{1541372629, 1541379924, 1541382930, 1541389423, 1541398637, 1541408022, 1541418563, 1541428900, 1541439501, 1542924244, 1545434420, 1545474895, 1545518017, 1547639955, 1548644767, 1548671748, 1548716462};

//const auto BM_SCAN_VALUES = BM_VAL_CAPTAIN_ID.size();

// quantile between benchmark values (int table)
// timestamp values --> unix timestamp
// [0.0001, 0.01, 0.02, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.9, 1.0]
///////////////////////////////
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_CAPTAIN_ID {{451745, 451746}, {435398, 460428}, {422254, 464467}, {403006, 496125}, {298194, 538889}, {229320, 570621}, {176022, 616628}, {140460, 695666}, {100762, 748517}, {57069, 901080}, {24882, 954443}, {11787, 1156169}, {6362, 1204047}, {4873, 1216443}, {4115, 1233112}, {1628, 1308975}, {4, 1414788}};
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_CAPTAIN_STATUS {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}};
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_LATITUDE {{251244097, 251244248}, {251230315, 251257525}, {251209843, 251282647}, {251163169, 251357987}, {251129117, 251483861}, {251088242, 251602810}, {251030861, 251724729}, {250980490, 251843563}, {250938817, 251886562}, {250872227, 251966912}, {250775666, 252081200}, {250699666, 252164364}, {250573540, 252290339}, {250500198, 252342398}, {250455204, 252437204}, {250299799, 252579811}, {102191832, 601671321}};
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_LONGITUDE {{552470640, 552471070}, {552454391, 552489962}, {552431658, 552503086}, {552373107, 552528396}, {552248024, 552598718}, {552163676, 552647236}, {552072508, 552692599}, {552030164, 552730356}, {551962965, 552761497}, {551792718, 552806365}, {551685925, 552898482}, {551549474, 553156638}, {551481989, 553443178}, {551450469, 553532204}, {551423584, 553640991}, {551349072, 553979534}, {-2135243575, 2123391399}};
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_TIMESTAMP {{1545434360, 1545434475}, {1544130507, 1545440081}, {1544125430, 1545444223}, {1544112365, 1545451370}, {1544084375, 1545459820}, {1544070890, 1545467594}, {1542924244, 1545474895}, {1542878145, 1545483158}, {1542855912, 1545490798}, {1541439501, 1545518018}, {1541428900, 1547639955}, {1541418563, 1548644767}, {1541408022, 1548658352}, {1541403370, 1548665723}, {1541398637, 1548671748}, {1541389423, 1548687353}, {1541372479, 1548716462}};

// 400 mio pings table

// quantile benchmark values (int table)
// timestamp values --> unix timestamp
// [0.0001, 0.01, 0.02, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.9, 1.0]
///////////////////////////////
//const auto BM_VAL_CAPTAIN_ID = std::vector{4, 511, 1051, 2156, 5075, 11309, 26152, 51264, 71463, 153884, 261690, 444765, 681250, 830979, 951600, 1209929, 1419878};
//const auto BM_VAL_CAPTAIN_STATUS = std::vector{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};
//const auto BM_VAL_LATITUDE = std::vector{243318475, 249128864, 249867308, 250278868, 250441577, 250552193, 250681596, 250763657, 250831026, 251008836, 251205836, 251695319, 251959544, 252079334, 252166598, 252442895, 601671321};
//const auto BM_VAL_LONGITUDE = std::vector{543464250, 550118701, 551136315, 551335287, 551413500, 551467322, 551533330, 551663587, 551771444, 552061945, 552444439, 552686763, 552805030, 552905709, 553158162, 553615144, 2137369825};
//const auto BM_VAL_TIMESTAMP = std::vector{1541029693, 1541117270, 1541190288, 1541473344, 1541871716, 1542301289, 1542701374, 1543132978, 1543613596, 1544417407, 1545152896, 1545879041, 1546772391, 1547148186, 1547525107, 1548196911, 1548975682};

//const auto BM_SCAN_VALUES = BM_VAL_CAPTAIN_ID.size();

// quantile between benchmark values (int table)
// timestamp values --> unix timestamp
// [0.0001, 0.01, 0.02, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.9, 1.0]
///////////////////////////////
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_CAPTAIN_ID {{261690, 261690}, {257105, 265387}, {249105, 274829}, {232076, 313448}, {206962, 391070}, {172746, 413552}, {153884, 444765}, {135981, 491021}, {111718, 538905}, {71463, 681250}, {51264, 830979}, {26152, 951600}, {11309, 1157854}, {7739, 1188073}, {5075, 1209929}, {2156, 1267757}, {4, 1419878}};
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_CAPTAIN_STATUS {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}};
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_LATITUDE {{251205594, 251206088}, {251186996, 251229239}, {251170200, 251244361}, {251150524, 251307385}, {251111686, 251424324}, {251063392, 251567452}, {251008836, 251695319}, {250959444, 251817667}, {250916446, 251875782}, {250831026, 251959544}, {250763657, 252079334}, {250681596, 252166598}, {250552193, 252294631}, {250485765, 252349413}, {250441577, 252442895}, {250278868, 252584668}, {-348142571, 601671321}};
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_LONGITUDE {{552444244, 552444647}, {552421152, 552466309}, {552403773, 552484039}, {552334887, 552512683}, {552219192, 552585334}, {552133587, 552638424}, {552061945, 552686763}, {552006222, 552727166}, {551944417, 552758956}, {551771444, 552805030}, {551663587, 552905709}, {551533330, 553158162}, {551467322, 553438739}, {551440083, 553526687}, {551413500, 553615144}, {551335287, 553972207}, {-2144769047, 2137369825}};
//const std::vector<std::vector<int>> BM_BETWEEN_VAL_TIMESTAMP {{1545152588, 1545153213}, {1545125220, 1545195923}, {1545101636, 1545220279}, {1544986222, 1545315256}, {1544806072, 1545537953}, {1544611869, 1545697161}, {1544417407, 1545879041}, {1544199073, 1546078740}, {1544001704, 1546335968}, {1543613596, 1546772391}, {1543132978, 1547148186}, {1542701374, 1547525107}, {1542301289, 1547873717}, {1542094512, 1548046293}, {1541871716, 1548196911}, {1541473344, 1548555299}, {1541026811, 1548975682}};

///////////////////////////////
// methods
///////////////////////////////

std::string get_table_name(const std::string table_name, const std::string encoding) {
  return table_name + "_encoding_" + encoding;
} 

}  // namespace

///////////////////////////////
// Fixtures
///////////////////////////////

namespace opossum {

class TableWrapper;

// Defining the base fixture class
class SplitBenchmarkFixture : public MicroBenchmarkBasicFixture {
 public:
  void SetUp(::benchmark::State& state) {
    auto& storage_manager = Hyrise::get().storage_manager;

    // Generate tables
    if (!_data_generated) {

      // file for table stats
      std::ofstream segment_meta_data_csv_file(SEGMENT_META_DATA_FILE);
      segment_meta_data_csv_file << "TABLE_NAME,COLUMN_ID,ENCODING,CHUNK_ID,ROW_COUNT,SIZE_IN_BYTES\n";

      std::ofstream index_meta_data_csv_file(INDEX_META_DATA_FILE);
      index_meta_data_csv_file << "TABLE_NAME,COLUMN_ID,ENCODING,CHUNK_ID,ROW_COUNT,SIZE_IN_BYTES\n"; 

      for (const opossum::SegmentEncodingSpec & encoding : CHUNK_ENCODINGS) {
        const auto encoding_type = encoding_type_to_string.left.at(encoding.encoding_type);
        const auto new_table_name = get_table_name(TABLE_NAME_PREFIX, encoding_type);

        auto new_table = load_table(TBL_FILE, CHUNK_SIZE);
        auto table_wrapper = std::make_shared<TableWrapper>(new_table);
        table_wrapper->execute();
        const auto chunk_encoding_spec = ChunkEncodingSpec(table_wrapper->get_output()->column_count(), {encoding});
        
        ChunkEncoder::encode_all_chunks(new_table, chunk_encoding_spec);

        storage_manager.add_table(new_table_name, new_table);
        std::cout << "Created table: " << new_table_name << std::endl;

        for (auto column_id = ColumnID{0}; column_id < new_table->column_count(); ++column_id) {
          for (auto chunk_id = ChunkID{0}, end = new_table->chunk_count(); chunk_id < end;  ++chunk_id) {
            const auto& chunk = new_table->get_chunk(chunk_id);
            const auto& segment = chunk->get_segment(column_id);
            segment_meta_data_csv_file << new_table_name << "," << new_table->column_name(column_id) << "," << encoding << "," << chunk_id << "," << CHUNK_SIZE << "," << segment->memory_usage(MemoryUsageCalculationMode::Full) << "\n";
          }
        }

        // create index for each chunk and each segment 
        if (CREATE_INDEX && encoding.encoding_type == EncodingType::Dictionary) {
          std::cout << "Creating indexes: ";
          const auto chunk_count = new_table->chunk_count();
          const auto column_count = new_table->column_count();
              
          for (auto chunk_id = ChunkID{0}; chunk_id < chunk_count; ++chunk_id) {
            for (auto column_id = ColumnID{0}; column_id < column_count; ++column_id) {
              const auto& index = new_table->get_chunk(chunk_id)->create_index<GroupKeyIndex>(std::vector<ColumnID>{column_id});
              index_meta_data_csv_file << new_table_name << "," << new_table->column_name(column_id) << ","<< encoding << ","<< chunk_id << "," << CHUNK_SIZE << "," << index->memory_consumption() << "\n";
            }
          }
          std::cout << "done " << std::endl;
        }
      }
      segment_meta_data_csv_file.close();
      index_meta_data_csv_file.close();
    }
    _data_generated = true;
  }

  // Required to avoid resetting of StorageManager in MicroBenchmarkBasicFixture::TearDown()
  void TearDown(::benchmark::State&) {}

  inline static bool _data_generated = false;

};

///////////////////////////////
// benchmarks
///////////////////////////////


BENCHMARK_DEFINE_F(SplitBenchmarkFixture, BM_Split_LessThanEqualsPerformance)(benchmark::State& state) {
  auto& storage_manager = Hyrise::get().storage_manager;
  
  const auto encoding = CHUNK_ENCODINGS[state.range(0)];
  const auto scan_column_index = state.range(1);
  const auto scan_column = SCAN_COLUMNS[scan_column_index];
  const auto search_value_index = state.range(2);


  const auto encoding_type = encoding_type_to_string.left.at(encoding.encoding_type);
  const auto table_name = get_table_name(TABLE_NAME_PREFIX, encoding_type);
  
  auto table = storage_manager.get_table(table_name);

  const auto scan_column_id = table->column_id_by_name(scan_column);
  //const auto order_by_column_id = table->column_id_by_name(order_by_column);
  auto operand = pqp_column_(scan_column_id, table->column_data_type(scan_column_id), false, scan_column);

  // scan
  std::shared_ptr<BinaryPredicateExpression> predicate;
  // should by nicer dicer
  if (scan_column_index == 0) {predicate = std::make_shared<BinaryPredicateExpression>(PredicateCondition::LessThanEquals, operand, value_(BM_VAL_YEAR[search_value_index]));}
  if (scan_column_index == 1) {predicate = std::make_shared<BinaryPredicateExpression>(PredicateCondition::LessThanEquals, operand, value_(BM_VAL_MONTH[search_value_index]));}
  if (scan_column_index == 2) {predicate = std::make_shared<BinaryPredicateExpression>(PredicateCondition::LessThanEquals, operand, value_(BM_VAL_DAY[search_value_index]));}
  if (scan_column_index == 3) {predicate = std::make_shared<BinaryPredicateExpression>(PredicateCondition::LessThanEquals, operand, value_(BM_VAL_HOUR[search_value_index]));}
  if (scan_column_index == 4) {predicate = std::make_shared<BinaryPredicateExpression>(PredicateCondition::LessThanEquals, operand, value_(BM_VAL_MINUTE[search_value_index]));}
  if (scan_column_index == 5) {predicate = std::make_shared<BinaryPredicateExpression>(PredicateCondition::LessThanEquals, operand, value_(BM_VAL_SECOND[search_value_index]));}

  auto table_wrapper = std::make_shared<TableWrapper>(table);
  table_wrapper->execute();

  const auto warm_up_table_scan = std::make_shared<TableScan>(table_wrapper, predicate);
  warm_up_table_scan->execute();
  
  for (auto _ : state) {
    const auto table_scan = std::make_shared<TableScan>(table_wrapper, predicate);
    table_scan->execute();
  }
}

BENCHMARK_DEFINE_F(SplitBenchmarkFixture, BM_Split_BetweenPerformance)(benchmark::State& state) {
  auto& storage_manager = Hyrise::get().storage_manager;

  const auto encoding = CHUNK_ENCODINGS[state.range(0)];
  const auto scan_column_index = state.range(1);
  const auto scan_column = SCAN_COLUMNS[scan_column_index];
  const auto search_value_index = state.range(2);

  const auto encoding_type = encoding_type_to_string.left.at(encoding.encoding_type);
  const auto table_name = get_table_name(TABLE_NAME_PREFIX, encoding_type);
  
  auto table = storage_manager.get_table(table_name);

  const auto scan_column_id = table->column_id_by_name(scan_column);
  const auto column_expression =  pqp_column_(scan_column_id, table->column_data_type(scan_column_id), false, scan_column);

  auto table_wrapper = std::make_shared<TableWrapper>(table);
  table_wrapper->execute();

  // setting up right value (i.e., the search value)
  std::shared_ptr<BetweenExpression> predicate;
  // should by nicer dicer
  if (scan_column_index == 0) { predicate = std::make_shared<BetweenExpression>(PredicateCondition::BetweenInclusive, column_expression, value_(BM_BETWEEN_VAL_YEAR[search_value_index][0]), value_(BM_BETWEEN_VAL_YEAR[search_value_index][1])); }
  if (scan_column_index == 1) { predicate = std::make_shared<BetweenExpression>(PredicateCondition::BetweenInclusive, column_expression, value_(BM_BETWEEN_VAL_MONTH[search_value_index][0]), value_(BM_BETWEEN_VAL_MONTH[search_value_index][1])); }
  if (scan_column_index == 2) { predicate = std::make_shared<BetweenExpression>(PredicateCondition::BetweenInclusive, column_expression, value_(BM_BETWEEN_VAL_DAY[search_value_index][0]), value_(BM_BETWEEN_VAL_DAY[search_value_index][1])); }
  if (scan_column_index == 3) { predicate = std::make_shared<BetweenExpression>(PredicateCondition::BetweenInclusive, column_expression, value_(BM_BETWEEN_VAL_HOUR[search_value_index][0]), value_(BM_BETWEEN_VAL_HOUR[search_value_index][1])); }
  if (scan_column_index == 4) { predicate = std::make_shared<BetweenExpression>(PredicateCondition::BetweenInclusive, column_expression, value_(BM_BETWEEN_VAL_MINUTE[search_value_index][0]), value_(BM_BETWEEN_VAL_MINUTE[search_value_index][1])); }
  if (scan_column_index == 5) { predicate = std::make_shared<BetweenExpression>(PredicateCondition::BetweenInclusive, column_expression, value_(BM_BETWEEN_VAL_SECOND[search_value_index][0]), value_(BM_BETWEEN_VAL_SECOND[search_value_index][1])); }

  const auto warm_up_between_scan = std::make_shared<TableScan>(table_wrapper, predicate);
  warm_up_between_scan->execute();

  for (auto _ : state) {
    const auto between_scan = std::make_shared<TableScan>(table_wrapper, predicate);
    between_scan->execute();
  }
}

static void CustomArguments(benchmark::internal::Benchmark* b) {
  for (size_t encoding_id = 0; encoding_id < CHUNK_ENCODINGS.size(); ++encoding_id) {
    for (size_t scan_column_id = 0; scan_column_id < SCAN_COLUMNS.size(); ++scan_column_id) {
      for (size_t scan_value_id = 0; scan_value_id < BM_SCAN_VALUES; ++scan_value_id)
      {
        b->Args({static_cast<long long>(encoding_id), static_cast<long long>(scan_column_id), static_cast<long long>(scan_value_id)});
      }
    }
  }
}
BENCHMARK_REGISTER_F(SplitBenchmarkFixture, BM_Split_LessThanEqualsPerformance)->Apply(CustomArguments);
BENCHMARK_REGISTER_F(SplitBenchmarkFixture, BM_Split_BetweenPerformance)->Apply(CustomArguments);


}  // namespace opossum