#ifndef GRAPH_EXCEPTION
#define GRAPH_EXCEPTION
#include <exception>
#include <iostream>
#include <utility>

// Shamelessly copied from https://stackoverflow.com/a/8152888

#define GRAPH_API_PANIC -9

class GraphException : public std::exception
{
   public:
    /** Constructor (C strings).
     *  @param message C-style string error message.
     *                 The string contents are copied upon construction.
     *                 Hence, responsibility for deleting the char* lies
     *                 with the caller.
     */
    explicit GraphException(const char *message) : msg_(message) {}

    /** Constructor (C++ STL strings).
     *  @param message The error message.
     */
    explicit GraphException(std::string message) : msg_(std::move(message)) {}

    /** Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~GraphException() noexcept {}

    /** Returns a pointer to the (constant) error description.
     *  @return A pointer to a const char*. The underlying memory
     *          is in posession of the Exception object. Callers must
     *          not attempt to free the memory.
     */
    virtual const char *what() const noexcept { return msg_.c_str(); }

   protected:
    /** Error message.
     */
    std::string msg_;
};
#endif