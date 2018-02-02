/**
 * @file
 * @brief Definition of Object base class
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

/**
 * @defgroup Objects Objects
 * @brief Collection of objects passed around between modules
 */

#ifndef ALLPIX_OBJECT_H
#define ALLPIX_OBJECT_H

#include <iostream>

#include <TObject.h>

namespace allpix {
    template <typename T> class Message;

    /**
     * @ingroup Objects
     * @brief Base class for internal objects
     */
    class Object : public TObject {
    public:
        /**
         * @brief Required default constructor
         */
        Object() = default;
        /**
         * @brief Required virtual destructor
         */
        ~Object() override = default;

        /// @{
        /**
         * @brief Use default copy behaviour
         */
        Object(const Object&);
        Object& operator=(const Object&);
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        Object(Object&&);
        Object& operator=(Object&&);
        /// @}

        /**
         * @brief ROOT class definition
         */
        ClassDef(Object, 2);

        /** Overloaded ostream operator for printing of object data
        */
        friend std::ostream& operator<<(std::ostream& out, const Object& obj) {
            obj.print(out);
            return out;
        }

    protected:
        /**
         * @brief Print an ASCII representation of this Object to the given stream
         * @param out Stream to print to
         */
        virtual void print(std::ostream& out) const { out << "<unknown object>"; };
    };
} // namespace allpix

#endif
