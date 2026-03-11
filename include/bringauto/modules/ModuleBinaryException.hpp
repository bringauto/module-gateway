#pragma once

#include <stdexcept>



namespace bringauto::modules {

/**
 * @brief Exception thrown when the module binary fails to start or become ready
 */
class ModuleBinaryException : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

}
