/**
 * @file
 * @brief Definition of PulseTransfer module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/PropagatedCharge.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to combine all charges induced at every pixel.
     *
     * This modules takes pulses created by the TransientPropagation module and compines the individual induced charges from
     * each of the PropagatedCharges at every pixel into PixelCharges.
     */
    class PulseTransferModule : public Module {
    public:
        /**
         * @brief Constructor for the PulseTransfer module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        PulseTransferModule(Configuration& config, Messenger* messenger, const std::shared_ptr<Detector>& detector);

        /**
         * @brief Combine pulses from propagated charges and transfer them to the pixels
         */
        void run(unsigned int) override;

    private:
        bool output_plots_{}, output_pulsegraphs_{};

        // General module members
        std::shared_ptr<Detector> detector_;
        Messenger* messenger_;
        std::shared_ptr<PropagatedChargeMessage> message_;
    };
} // namespace allpix