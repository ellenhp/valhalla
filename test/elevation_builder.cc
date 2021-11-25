#include "test.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <string>
#include <unordered_set>

#include <prime_server/prime_server.hpp>

#include "baldr/tilehierarchy.h"
#include "filesystem.h"
#include "midgard/pointll.h"
#include "mjolnir/elevationbuilder.h"
#include "mjolnir/graphtilebuilder.h"
#include "mjolnir/valhalla_add_elevation_utils.h"
#include "pixels.h"
#include "skadi/sample.h"
#include "tile_server.h"

namespace {
// meters to resample shape to.
// see elevationbuilder.cc for details
constexpr double POSTING_INTERVAL = 60;
const std::string src_dir{"test/data/"};
const std::string elevation_local_src{"elevation_src"};
const std::string src_path = src_dir + elevation_local_src;

zmq::context_t context;

std::unordered_set<PointLL> get_coord(const std::string& tile_dir, const std::string& tile);
std::vector<std::string> full_to_relative_path(const std::string& root_dir,
                                               std::vector<std::string>&& paths) {
  if (root_dir.empty() || paths.empty())
    return std::move(paths);

  std::vector<std::string> res;
  res.reserve(paths.size());
  for (auto&& path : paths) {
    auto pos = path.rfind(root_dir);
    if (pos == std::string::npos) {
      res.push_back(std::move(path));
      continue;
    }

    res.push_back(path.substr(pos + root_dir.size()));
  }

  return res;
}

TEST(Filesystem, full_to_relative_path_valid_input) {
  struct test_desc {
    std::string path;
    std::string remove_pattern;
    std::string res;
  };
  std::vector<test_desc> tests =
      {{"/valhalla-internal/build/test/data/utrecht_tiles/0/003/196.gp",
        "/valhalla-internal/build/test/data/utrecht_tiles", "/0/003/196.gp"},
       {"/valhalla-internal/build/test/data/utrecht_tiles/1/051/305.gph",
        "/valhalla-internal/build/test/data/utrecht_tiles", "/1/051/305.gph"},
       {"/utrecht_tiles/build/test/data/utrecht_tiles/1/051/305.gph", "utrecht_tiles",
        "/1/051/305.gph"},
       {"/utrecht_tiles/utrecht_tiles/utrecht_tiles/utrecht_tiles/utrecht_tiles/1/051/305.gph",
        "utrecht_tiles", "/1/051/305.gph"},
       {"/utrecht_tiles/build/test/data/utrecht_tiles/1/051/305.gph", "data",
        "/utrecht_tiles/1/051/305.gph"},
       {"/data/build/test/data/utrecht_tiles/1/051/305.gph", "data", "/utrecht_tiles/1/051/305.gph"},
       {"/valhalla-internal/build/test/data/utrecht_tiles/2/000/818/660.gph",
        "/valhalla-internal/build/test/data/utrecht_tiles", "/2/000/818/660.gph"},
       {"/valhalla-internal/build/test/data/utrecht_tiles/0/003/196.gp", "",
        "/valhalla-internal/build/test/data/utrecht_tiles/0/003/196.gp"},
       {"", "/valhalla-internal/build/test/data/utrecht_tiles", ""},
       {"/valhalla-internal/build/test/data/utrecht_tiles/2/000/818/660.gph", "/tmp",
        "/valhalla-internal/build/test/data/utrecht_tiles/2/000/818/660.gph"}};

  for (const auto& test : tests)
    EXPECT_EQ(full_to_relative_path(test.remove_pattern, {test.path}).front(), test.res);
}

TEST(Filesystem, full_to_relative_path_invalid_input) {
  struct test_desc {
    std::string path;
    std::string remove_pattern;
    std::string res;
  };

  std::vector<test_desc> tests =
      {{"/valhalla-internal/build/test/data/utrecht_tiles/0/003/196.gph", "1", "96.gph"},
       {"/valhalla-internal/build/test/data/utrecht_tiles/1/051/3o5.gph",
        "/valhalla-internal/build/test/data/utrecht_tiles", "/1/051/3o5.gph"},
       {"$", "/valhalla-internal/build/test/data/utrecht_tiles", "$"},
       {"/valhalla-internal/build/test/data/utrecht_tiles/0/003/196.gph",
        "/valhalla-internal/build/test/data/utrecht_tiles/0/003/196.gph", ""}};

  for (const auto& test : tests)
    EXPECT_EQ(full_to_relative_path(test.remove_pattern, {test.path}).front(), test.res);
}

/**
 * @brief Removes all the content of the directory.
 * */
inline bool clear(const std::string& dir) {
  if (!filesystem::exists(dir))
    return false;

  if (!filesystem::is_directory(dir))
    return filesystem::remove(dir);

  for (filesystem::recursive_directory_iterator i(dir), end; i != end; ++i) {
    if (filesystem::exists(i->path()))
      filesystem::remove_all(i->path());
  }

  return true;
}

TEST(Filesystem, clear_valid_input) {
  std::vector<std::string> tests{"/tmp/save_file_input/utrecht_tiles/0/003/196.gph",
                                 "/tmp/save_file_input/utrecht_tiles/1/051/305.gph",
                                 "/tmp/save_file_input/utrecht_tiles/2/000/818/660.gph"};

  for (const auto& test : tests)
    (void)filesystem::save(test);

  EXPECT_FALSE(filesystem::get_files("/tmp/save_file_input/utrecht_tiles").empty());

  EXPECT_TRUE(clear("/tmp/save_file_input/utrecht_tiles/2/000/818/660.gph"));
  EXPECT_TRUE(clear("/tmp/save_file_input/"));

  EXPECT_TRUE(filesystem::get_files("/tmp/save_file_input/utrecht_tiles").empty());
}

TEST(Filesystem, clear_invalid_input) {
  EXPECT_FALSE(clear(".foobar"));
}

struct ElevationDownloadTestData {
  ElevationDownloadTestData(const std::string& dir_dst) : m_dir_dst{dir_dst} {
    load_tiles();
  }

  void load_tiles() {
    for (auto&& ftile : full_to_relative_path(m_dir_dst, filesystem::get_files(m_dir_dst))) {
      if (ftile.find("gph") != std::string::npos && !m_test_tile_names.count(ftile))
        m_test_tile_names.insert(std::move(ftile));
    }
  }

  std::vector<valhalla::baldr::GraphId> m_test_tile_ids;
  std::unordered_set<std::string> m_test_tile_names;
  std::string m_dir_dst;
};

struct TestableSample : public valhalla::skadi::sample {
  static uint16_t get_tile_index(const PointLL& coord) {
    return valhalla::skadi::sample::get_tile_index(coord);
  }
  static std::string get_hgt_file_name(uint16_t index) {
    return valhalla::skadi::sample::get_hgt_file_name(index);
  }
};

std::unordered_set<PointLL> get_coord(const std::string& tile_dir, const std::string& tile) {
  if (tile_dir.empty() || tile.empty())
    return {};

  valhalla::mjolnir::GraphTileBuilder tilebuilder(tile_dir, GraphTile::GetTileId(tile_dir + tile),
                                                  true);
  tilebuilder.header_builder().set_has_elevation(true);

  uint32_t count = tilebuilder.header()->directededgecount();
  std::unordered_set<uint32_t> cache;
  cache.reserve(2 * count);

  std::unordered_set<PointLL> result;
  for (uint32_t i = 0; i < count; ++i) {
    // Get a writeable reference to the directed edge
    const DirectedEdge& directededge = tilebuilder.directededge_builder(i);
    // Get the edge info offset
    uint32_t edge_info_offset = directededge.edgeinfo_offset();
    if (cache.count(edge_info_offset))
      continue;

    cache.insert(edge_info_offset);
    // Get the shape and length
    auto shape = tilebuilder.edgeinfo(&directededge).shape();
    auto length = directededge.length();
    if (!directededge.tunnel() && directededge.use() != Use::kFerry) {
      // Evenly sample the shape. If it is really short or a bridge just do both ends
      std::vector<PointLL> resampled;
      if (length < POSTING_INTERVAL * 3 || directededge.bridge()) {
        resampled = {shape.front(), shape.back()};
      } else {
        resampled = valhalla::midgard::resample_spherical_polyline(shape, POSTING_INTERVAL);
      }

      for (auto&& el : resampled)
        result.insert(std::move(el));
    }
  }

  return result;
}

TEST(ElevationBuilder, get_coord) {
  EXPECT_TRUE(get_coord("test/data/elevationbuild/tile_dir", "").empty());
  EXPECT_TRUE(get_coord("", "/0/003/196.gph").empty());
  EXPECT_FALSE(get_coord("test/data/elevationbuild/tile_dir", "/0/003/196.gph").empty());
}

TEST(ElevationBuilder, get_tile_ids) {
  auto config = test::make_config("test/data", {}, {});

  EXPECT_TRUE(valhalla::mjolnir::get_tile_ids(config, {}).empty());

  config.erase("mjolnir.tile_dir");
  // check if config contains tile_dir
  EXPECT_TRUE(valhalla::mjolnir::get_tile_ids(config, {"/0/003/196.gph"}).empty());

  // check if config contains elevation dir
  config.erase("additional_data.elevation");
  EXPECT_TRUE(valhalla::mjolnir::get_tile_ids(config, {"/0/003/196.gph"}).empty());

  config.put<std::string>("additional_data.elevation", "test/data/elevation_dst/");
  config.put<std::string>("mjolnir.tile_dir", "test/data/parser_tiles");
  // check if tile is from the specified tile_dir (it is not)
  EXPECT_TRUE(valhalla::mjolnir::get_tile_ids(config, {"/0/003/196.gph"}).empty());

  config.put<std::string>("mjolnir.tile_dir", "test/data/elevationbuild/tile_dir");
  EXPECT_FALSE(valhalla::mjolnir::get_tile_ids(config, {"/0/003/196.gph"}).empty());
}

TEST(ElevationBuilder, test_loaded_elevations) {
  const auto& config = test::
      make_config("test/data",
                  {{"additional_data.elevation_url",
                    "127.0.0.1:38004/route-tile/v1/{tilePath}?version=%version&access_token=%token"},
                   {"mjolnir.tile_dir", "test/data/tile_src"},
                   {"additional_data.elevation_dir", elevation_local_src},
                   {"additional_data.elevation", "test/data/elevation_dst/"},
                   {"mjolnir.concurrency", "5"}});

  const auto& tile_dir = config.get<std::string>("mjolnir.tile_dir");
  ElevationDownloadTestData params{tile_dir};
  std::unordered_set<PointLL> coords_storage;
  for (const auto& tile : params.m_test_tile_names) {
    for (const auto& coord : get_coord(tile_dir, tile)) {
      coords_storage.insert(coord);
    }
  }

  std::vector<std::string> src_elevations;
  std::unordered_set<std::string> seen;
  for (const auto& coord : coords_storage) {
    auto elev = TestableSample::get_hgt_file_name(TestableSample::get_tile_index(coord));
    if (!seen.count(elev)) {
      seen.insert(elev);
      src_elevations.push_back(std::move(elev));
    }
  }

  ASSERT_FALSE(src_elevations.empty()) << "Fail to create any source elevations";

  for (const auto& elev : src_elevations)
    EXPECT_TRUE(filesystem::save(src_path + elev));

  const auto& dst_dir = config.get<std::string>("additional_data.elevation");
  valhalla::mjolnir::ElevationBuilder::Build(config,
                                             valhalla::mjolnir::get_tile_ids(config,
                                                                             params
                                                                                 .m_test_tile_names));

  ASSERT_TRUE(filesystem::exists(dst_dir));

  const auto& elev_paths = filesystem::get_files(dst_dir);
  ASSERT_FALSE(elev_paths.empty());

  std::unordered_set<std::string> dst_elevations;
  for (const auto& elev : elev_paths)
    dst_elevations.insert(elev);

  clear(dst_dir);

  for (const auto& elev : src_elevations) {
    EXPECT_TRUE(
        std::find_if(std::begin(dst_elevations), std::end(dst_elevations), [&elev](const auto& file) {
          return file.find(elev) != std::string::npos;
        }) != std::end(dst_elevations));
  }

  clear(src_path);
}

} // namespace

class HttpElevationsEnv : public ::testing::Environment {
public:
  void SetUp() override {
    valhalla::test_tile_server_t server;
    server.set_url("127.0.0.1:38004");
    server.start(src_dir, context);
  }

  void TearDown() override {
  }
};

int main(int argc, char* argv[]) {
  testing::AddGlobalTestEnvironment(new HttpElevationsEnv);
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}