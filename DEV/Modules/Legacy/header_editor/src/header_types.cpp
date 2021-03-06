#include "header_types.h"

Header::Header() {}
Header::~Header() {}

auto GetDefaultFields() -> std::vector<HeaderEntry>
{
  using std::string;
  using field_t = HeaderEntry;

  std::vector<HeaderEntry> list{};

  auto add_entry =
      [&](bool flag, string path, string val = "", string units = "")
  {
    field_t field{path, val, units, flag};
    list.push_back(std::move(field));
  };

  // ---------- FILE LOCATION ---------- //
  add_entry(true, "//Web_Location", "http://server.web/your/folder");

  // ---------- OBJECT ---------- //
  add_entry(true, "//Object/Name");
  add_entry(true, "//Object/Identifier");
  add_entry(true, "//Object/Description");

  // ---------- DEVICE ---------- //
  add_entry(true, "//Device/Identifier");
  // A model number for the device, maybe internally assigned
  add_entry(true, "//Device/Make");
  // CHNET-UNSAM, CHNET-LABEC, CHNET-NYUAD

  // ---------- SESSION ---------- //
  add_entry(true, "//Session/Identifier");
  // Session for a given series of analysis on an object
  add_entry(true, "//Session/Investigation_Type");
  // Test, Calibration, Point DAQ, Scan DAQ
  // Or maybe like purpose

  // ---------- ANALYSIS METADATA ---------- //
  add_entry(true, "//Metadata/Analysis_Identifier");
  // Specific analysis done
  add_entry(true, "//Metadata/Operator");

  // Entered by the program
  add_entry(true, "//Metadata/Date");
  add_entry(true, "//Metadata/Time");

//  add_entry(true, "//Metadata/Measurement_Unit");
  // No idea what these next ones are
  add_entry(true, "//Metadata/Sample_Area_File");
  add_entry(true, "//Metadata/Sampling_Note");
  add_entry(true, "//Metadata/Note");

  // ---------- ANALYSIS INFO ---------- //
  add_entry(true, "//Info/Tube/Voltage");
  add_entry(true, "//Info/Tube/Current");
  add_entry(true, "//Info/Tube/Anode", "Molybdenum");

  // ---------- HISTOGRAM ---------- //
  //  Histogram alignment will use the calibration parameters of the detectors
  add_entry(true, "//Histogram/Detector_Identifier");
  add_entry(true, "//Histogram/Calibration_Param_0");
  add_entry(true, "//Histogram/Calibration_Param_1");
  add_entry(true, "//Histogram/Calibration_Param_2");

  // These parameters are entered by the program -------------------- //
  // Parameters only relevant for DAQ
  add_entry(true, "//Info/Scan/Physical_X_Start");
  add_entry(true, "//Info/Scan/Physical_Y_Start");
  add_entry(true, "//Info/Scan/Physical_X_Step");
  add_entry(true, "//Info/Scan/Physical_Y_Step");
  add_entry(true, "//Info/Scan/Velocity");
  add_entry(true, "//Info/Scan/Dwell_Time");
  add_entry(true, "//Info/Image/Width");
  add_entry(true, "//Info/Image/Height");
  add_entry(true, "//Info/Image/Pixels");



  // We need a units attribute

  // We return by copy elision
  return list;
}

auto XMLHeader::MakeDefault() -> void
{
  fields_ = GetDefaultFields();

  pugi::xml_document doc;

  // Generate XML declaration
  auto declarationNode = doc.append_child(pugi::node_declaration);
  declarationNode.append_attribute("version")    = "1.0";
  declarationNode.append_attribute("encoding")   = "utf-8";


  auto root = doc.append_child("Analysis_Header");
  pugi::xml_node field;
  for (auto & fields : fields_)
  {
    std::string node_name = fields.path;
    node_name.erase(0,2);

    pugi::xml_node node;
    pugi::xpath_node xnode;
    auto pos = node_name.find_first_of('/');
    xnode = root.select_node(fields.path.c_str());
    if (!xnode) {
      node = root.append_child(node_name.substr(0, pos).c_str());
    }
    node_name.erase(0, pos);
    do
    {
      auto pos = node_name.find_first_of('/');
      xnode = root.select_node(fields.path.c_str());
      if (!xnode) {
        node = root.append_child(node_name.substr(0, pos).c_str());
      }
      node_name.erase(0, pos);
      pos = node_name.find_first_of('/', pos);
    } while (pos != std::string::npos);

    node.append_child(pugi::node_pcdata).set_value(fields.value.c_str());
    if (!fields.units.empty())
    {
      node.append_attribute("units").set_value(fields.units.c_str());
    }
    if ((int)(fields.flags & EntryFlags::IS_REQUIRED))
    {
      node.append_attribute("required").set_value("true");
    }
  }


//  doc.print(std::cout);

  std::cout << "Saving result: "
            << doc.save_file("/home/frao/Desktop/test.xml")
            << std::endl;
}



// The XML Format

// <XRF Analysis>
// <Analysis Header>
// Analysis File Location URL


// </Analysis Header>

// <AnalysisData>
// ...
// ...
//...
// </AnalysisData>
