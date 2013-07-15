#include <rapidxml.hpp>

#include <sstream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>

extern "C" {
  #include "gps.h"
}

#include "util.h"

const size_t kMaxFileSize = 100 * 1024 * 1024;

class GPX {
 public:
  class Trackpoint {
   public:
    enum FixType {
      ThreeD,
      DGPS,
      Unknown
    };

   public:
    Trackpoint(const float         lat,
               const float         lon,
               const struct tm     timestamp,
               const float         elevation = 0.0f,
               const std::string  &fix       = "") :
      lat(lat), lon(lon), timestamp(timestamp), elevation(elevation),
      fix(StringToFixType(fix)) {}

   protected:
    FixType StringToFixType(const std::string &s_fix) {
      if (s_fix == "3d")
        return ThreeD;
      else if (s_fix == "dgps")
        return DGPS;
      else
        return Unknown;
    }

   public:
    float      lat, lon, elevation;
    struct tm  timestamp;
    FixType    fix;
  };

  typedef std::vector<Trackpoint> TpVector;

 public:
  static GPX* Construct(const std::string &path) {
    if (! FileExists(path)) {
      std::cerr << "'" << path << "' does not exist." << std::endl;
      return nullptr;
    }

    // get file size
    const size_t file_size = FileSize(path.c_str());
    if (file_size == 0) {
      std::cerr << "File '" << path << "' is empty." << std::endl;
      return nullptr;
    }
    if (file_size > kMaxFileSize) {
      std::cerr << "File '" << path << "' is too big." << std::endl;
      return nullptr;
    }

    // open file
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
      std::cerr << "Cannot open '" << path << "'." << std::endl;
      return nullptr;
    }

    // read data
    char *gpx_data = (char*)malloc(file_size);
    if (gpx_data == nullptr) {
      std::cerr << "Out of memory when reading '" << path << "'." << std::endl;
      return nullptr;
    }
    const ssize_t retval = read(fd, gpx_data, file_size);
    if (retval != static_cast<ssize_t>(file_size)) {
      std::cerr << "Failed to read '" << path << "'." << std::endl;
      return nullptr;
    }

    // parse the data
    using namespace rapidxml;

    GPX *gpx = new GPX;
    xml_document<> doc;
    doc.parse<0>(gpx_data);

    // read the data nodes
    if (strncmp(doc.first_node()->name(), "gpx", 3) != 0) {
      std::cerr << "Unrecognized format in '" << path << "'" << std::endl;
      return nullptr;
    }

    unsigned int found_tracks = 0;
    xml_node<> *gpx_root = doc.first_node();
    for (xml_node<> *node = gpx_root->first_node("trk"); node;
         node = node->next_sibling("trk")) {
      xml_node<> *name_node = node->first_node("name");
      std::cout << "reading track: " << ((name_node != nullptr)
                                           ? name_node->first_node()->value()
                                           : "<unnamed>") << " ... ";
      std::cout.flush();
      ++found_tracks;

      if (! ReadTrack(node, gpx)) {
        std::cerr << "Failed to read track." << std::endl;
        return nullptr;
      }

      std::cout << "done" << std::endl;
    }

    std::cout << "found " << found_tracks << " tracks, "
              << "containing " << gpx->PointCount() << " trackpoints"
              << std::endl;

    return gpx;
  }

  static bool ReadTrack(const rapidxml::xml_node<> *trk_node, GPX* gpx) {
    using namespace rapidxml;

    for (xml_node<> *trkseg_node = trk_node->first_node("trkseg"); trkseg_node;
         trkseg_node = trkseg_node->next_sibling()) {
      for (xml_node<> *trkpt_node = trkseg_node->first_node("trkpt"); trkpt_node;
           trkpt_node = trkpt_node->next_sibling()) {
        if (!ReadTrackpoint(trkpt_node, gpx)) {
          std::cerr << "Failed to read trackpoint." << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  static bool ReadTrackpoint(const rapidxml::xml_node<> *trkpt_node, GPX *gpx) {
    using namespace rapidxml;

    xml_attribute<> *lat_attr = trkpt_node->first_attribute("lat");
    xml_attribute<> *lon_attr = trkpt_node->first_attribute("lon");
    if (lat_attr == nullptr || lon_attr == nullptr) {
      std::cerr << "No Lat/Lon information found in trackpoint" << std::endl;
      return false;
    }

    xml_node<> *elevation_node = trkpt_node->first_node("ele");
    xml_node<> *time_node      = trkpt_node->first_node("time");
    xml_node<> *fix_node       = trkpt_node->first_node("fix");

    if (time_node == NULL) {
      std::cerr << "No timestamp found in trackpoint" << std::endl;
      return false;
    }

    // convert the stuff into reasonable data
    const float lat = ToFloat(lat_attr->value());
    const float lon = ToFloat(lon_attr->value());
    if (lat == 0.0f || lon == 0.0f) {
      std::cerr << "Cannot read Lat/Lon information in trackpoint" << std::endl;
      return false;
    }

    const std::string s_timestamp = time_node->first_node()->value();
    const struct tm timestamp = ConvertTimestamp(s_timestamp);
    const float elevation = (elevation_node != nullptr)
                              ? ToFloat(elevation_node->first_node()->value())
                              : 0.0f;
    const std::string fix = (fix_node != nullptr)
                              ? fix_node->first_node()->value()
                              : "";

    // add a new trackpoint
    gpx->AddTrackpoint(Trackpoint(lat, lon, timestamp, elevation, fix));

    return true;
  }

  void AddTrackpoint(const Trackpoint &point) {
    trackpoints_.push_back(point);
  }

  unsigned int PointCount() const { return trackpoints_.size(); }

  const TpVector& trackpoints() const { return trackpoints_; }

 protected:
  GPX() {}

 private:
  TpVector trackpoints_;
};


int main(int argc, char **argv) {
  if (argc != 3) {
    std::stringstream ss;
    ss << "Usage: " << argv[0] << " <input gpx> <output gpx>";
    Die(ss.str());
  }

  GPX* gpx = GPX::Construct(argv[1]);
  if (!gpx) {
    std::stringstream ss;
    ss << "Cannot read file '" << argv[1] << "'";
    Die(ss.str());
  }

  delete gpx;
}
