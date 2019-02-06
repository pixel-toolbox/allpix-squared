/**
 * @file
 * @brief Implementation of module to read weighting fields
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied
 * verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities
 * granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "WeightingPotentialReaderModule.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

WeightingPotentialReaderModule::WeightingPotentialReaderModule(Configuration& config,
                                                               Messenger*,
                                                               std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {}

void WeightingPotentialReaderModule::init() {

    auto field_model = config_.get<std::string>("model");

    // Calculate thickness domain
    auto model = detector_->getModel();
    auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    auto thickness_domain = std::make_pair(sensor_max_z - model->getSensorSize().z(), sensor_max_z);

    // Calculate the potential depending on the configuration
    if(field_model == "init") {
        auto field_data = read_init_field(thickness_domain);

        // Calculate scale from field size and pixel pitch:
        auto pixel_size = model->getPixelSize();
        std::array<double, 2> field_scale{
            {std::get<2>(field_data)[0] / pixel_size.x(), std::get<2>(field_data)[1] / pixel_size.y()}};

        detector_->setWeightingPotentialGrid(
            std::get<0>(field_data), std::get<1>(field_data), field_scale, std::array<double, 2>{{0, 0}}, thickness_domain);
    } else if(field_model == "pad") {
        LOG(TRACE) << "Adding weighting potential from pad in plane condenser";

        // Get pixel implant size from the detector model:
        auto implant = model->getImplantSize();
        auto function = get_pad_potential_function(implant, thickness_domain);
        detector_->setWeightingPotentialFunction(function, thickness_domain, FieldType::CUSTOM);
    } else {
        throw InvalidValueError(config_, "model", "model should be 'init' or `pad`");
    }

    // Produce histograms if needed
    if(config_.get<bool>("output_plots", false)) {
        create_output_plots();
    }
}

/**
 * Implement weighting potential from doi:10.1016/j.nima.2014.08.044 and return it as a lookup function.
 */
FieldFunction<double>
WeightingPotentialReaderModule::get_pad_potential_function(const ROOT::Math::XYVector& implant,
                                                           std::pair<double, double> thickness_domain) {

    LOG(TRACE) << "Calculating function for the plane condenser weighting potential." << std::endl;

    return [implant, thickness_domain](const ROOT::Math::XYZPoint& pos) {
        // Calculate values of the "f" function
        auto f = [implant](double x, double y, double u) {
            // Calculate arctan fractions
            auto arctan = [](double a, double b, double c) {
                return std::atan(a * b / c / std::sqrt(a * a + b * b + c * c));
            };

            // Shift the x and y coordinates by plus/minus half the implant size:
            double x1 = x - implant.x() / 2;
            double x2 = x + implant.x() / 2;
            double y1 = y - implant.y() / 2;
            double y2 = y + implant.y() / 2;

            // Calculate arctan sum and return
            return arctan(x1, y1, u) + arctan(x2, y2, u) - arctan(x1, y2, u) - arctan(x2, y1, u);
        };

        // Transform into coordinate system with sensor between d/2 < z < -d/2:
        auto d = thickness_domain.second - thickness_domain.first;
        auto local_z = -pos.z() + thickness_domain.second;

        // Calculate the series expansion
        double sum = 0;
        for(int n = 1; n <= 100; n++) {
            sum += f(pos.x(), pos.y(), 2 * n * d - local_z) - f(pos.x(), pos.y(), 2 * n * d + local_z);
        }

        return (1 / (2 * M_PI) * (f(pos.x(), pos.y(), local_z) - sum));
    };
}

void WeightingPotentialReaderModule::create_output_plots() {
    LOG(TRACE) << "Creating output plots";

    auto steps = config_.get<size_t>("output_plots_steps", 500);
    auto position = config_.get<ROOT::Math::XYPoint>("output_plots_position", ROOT::Math::XYPoint(0, 0));

    auto model = detector_->getModel();

    double min = model->getSensorCenter().z() - model->getSensorSize().z() / 2.0;
    double max = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;

    // Create 1D histograms
    auto histogram = new TH1F("potential1d", "#phi_{w}/V_{w};z (mm);unit potential", static_cast<int>(steps), min, max);

    // Get the weighting potential at every index
    for(size_t j = 0; j < steps; ++j) {
        double z = min + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * (max - min);
        auto pos = ROOT::Math::XYZPoint(position.x(), position.y(), z);

        // Get potential from detector and fill the histogram
        auto potential = detector_->getWeightingPotential(pos, Pixel::Index(0, 0));
        histogram->Fill(z, potential);
    }

    // Create 2D histogram
    auto histogram2D = new TH2F("potential",
                                "#phi_{w}/V_{w};x (mm); z (mm); unit potential",
                                static_cast<int>(steps),
                                -1.5 * model->getPixelSize().x(),
                                1.5 * model->getPixelSize().x(),
                                static_cast<int>(steps),
                                min,
                                max);

    // Get the weighting potential at every index
    for(size_t j = 0; j < steps; ++j) {
        LOG_PROGRESS(INFO, "plotting") << "Plotting progress " << 100 * j * steps / (steps * steps) << "%";
        double z = min + ((static_cast<double>(j) + 0.5) / static_cast<double>(steps)) * (max - min);

        // Scan horizontally over three pixels (from -1.5 pitch to +1.5 pitch)
        for(size_t k = 0; k < steps; ++k) {
            double x = -1.5 * model->getPixelSize().x() +
                       ((static_cast<double>(k) + 0.5) / static_cast<double>(steps)) * 3 * model->getPixelSize().x();

            // Get potential from detector and fill histogram
            auto potential = detector_->getWeightingPotential(ROOT::Math::XYZPoint(x, 0, z), Pixel::Index(1, 0));
            histogram2D->Fill(x, z, potential);
        }
    }

    histogram->SetOption("hist");
    histogram2D->SetOption("colz");

    // Write the histogram to module file
    histogram->Write();
    histogram2D->Write();
}

/**
 * The field read from the INIT format are shared between module instantiations
 * using the static WeightingPotentialReaderModule::get_by_file_name method.
 */
FieldParser<double, 1> WeightingPotentialReaderModule::field_parser_("");
FieldData<double> WeightingPotentialReaderModule::read_init_field(std::pair<double, double> thickness_domain) {
    using namespace ROOT::Math;

    try {
        LOG(TRACE) << "Fetching weighting potential from init file";

        // Get field from file
        auto field_data = field_parser_.get_by_file_name(config_.getPath("file_name", true));

        // Check if electric field matches chip
        check_detector_match(std::get<2>(field_data), thickness_domain);

        LOG(INFO) << "Set weighting field with " << std::get<1>(field_data).at(0) << "x" << std::get<1>(field_data).at(1)
                  << "x" << std::get<1>(field_data).at(2) << " cells";

        // Return the field data
        return field_data;
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::runtime_error& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::bad_alloc& e) {
        throw InvalidValueError(config_, "file_name", "file too large");
    }
}

/**
 * @brief Check if the detector matches the file header
 */
void WeightingPotentialReaderModule::check_detector_match(std::array<double, 3> dimensions,
                                                          std::pair<double, double> thickness_domain) {
    auto xpixsz = dimensions[0];
    auto ypixsz = dimensions[1];
    auto thickness = dimensions[2];

    auto model = detector_->getModel();
    // Do a several checks with the detector model
    if(model != nullptr) {
        // Check field dimension in z versus the requested thickness domain:
        auto eff_thickness = thickness_domain.second - thickness_domain.first;
        if(std::fabs(thickness - eff_thickness) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Thickness of weighting potential is " << Units::display(thickness, "um")
                         << " but the depleted region is " << Units::display(eff_thickness, "um");
        }

        // Check that the total field size is n*pitch:
        if(std::fmod(xpixsz, model->getPixelSize().x()) > std::numeric_limits<double>::epsilon() ||
           std::fmod(ypixsz, model->getPixelSize().y()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Potential map size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"}) << ") but expecting a multiple of the pixel pitch ("
                         << Units::display(model->getPixelSize().x(), {"um", "mm"}) << ", "
                         << Units::display(model->getPixelSize().y(), {"um", "mm"}) << ")";
        }
    }
}