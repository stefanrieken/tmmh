tmmh: Typed Memory Managed Heap
===============================

Status
------
Allocate, release, reallocate and garbage collection are implemented and demo'able.

tmmh (pronounced: TIMMEH!!!) supports:
- Typed heap storage, with
- User-definable types
- Queryable type, size(!) and related information for any heap pointer
- Mark / sweep-based, explicit garbage collection
- Heap compression
- Pointer relocation
- Minimal administrative overhead targeted at >=32-bit machines (between 32 and 64 bits)


Applications
------------

I would recommend tmmh in any situation requiring either or both:
- typed and sized values on the heap
- garbage collection

Slightly counter-intuitively, storing a value together with its type (even with a finite set of types) is very useful in any situation requiring dynamic typing. This includes:

- LISP or Scheme-inspired languages
- Smalltalk-inspired languages
- Virtual machines

All the above examples would also benefit from having the garbage collector. Notice that these examples store both data and functions / methods (of any kind), for which tmmh could lend itself as well.

I would still recommend using tmmh if you would only use it for the luxury of typed and sized values, or just for garbage collection alone. In any case tmmh is very lightweight, and the garbage collector, already the most lightweight part, may be simply linked out if unused.

Finally, tmmh can be used to develop both hosted applications (where tmmh will piggy-back on malloc) and embedded ones (where can be used standalone). A hosted prototype can easily be converted on the spot to an embedded build.

The header
----------

Your common malloc-based heap is implemented as a linked list, and tmmh is essentially the same. Each tmmh entry is preceded by a header, in which a large amount of data is crammed.

tmmh intends to offer different header sizes. The smallest practically useful one is 16 bits. Next comes a very useful 32 bits header, and finally there is an overly roomy 64 bits version, which can nevertheless be very useful as a coverall size for both 32 and 64-bit systems. As a rule, always pick at least the header size that matches your target system's alignment.

In each case, the option exists to store half-header-width sized values directly in the header. The usefulness of this little extra feature really depends on the situation. For instance, on 64-bit systems you can store 32-bit typed integers in a single word. On 32- and 16-bit headers, that space might feel a bit more cramped (resp. 16- and 8-bit values), but may be useful in your situation.

The following is pseudo-code to show the basic structure for each of the header sizes.

	#ifdef HEADER_SIZE 16
	/* 16-bit aligned header with max 8-bit inline value */
	struct header {
		uint2_t flags; // in_USE, DIRECT_VALUE; note GC_MARKER == IN_USE flag to save space
		uint1_t bytes_unused;  // value size = node_size - bytes_unused (max 1)
		uint5_t datatype; // up to 32 user-definable datatypes
		union {
			uint8_t size; // 2-byte aligned, so multiply by 2 #ifdef MAXIMIZE_SIZE (max=512)
			uint8_t value; // if flagged as 'direct_value'
		}
	}
	#endif

	#ifdef HEADER_SIZE 32
	/* 32-bit aligned header with max 16-bit inline value */
	struct header {
		uint6_t flags; // IN_USE, GC_MARK, DIRECT_VALUE; 3 more user defined
		uint2_t bytes_unused; // value size = node_size - bytes_unused (max 3)
		uint8_t datatype; // up to 256 user-definable datatypes
		union {
			uint16_t size; // 4-byte aligned, so multiply by 4 #ifdef MAXIMIZE_SIZE (max=256k)
			uint16_t value; // if flagged as 'direct_value'
		}
	}

	#ifdef HEADER_SIZE 64
	/* 64-bit aligned header with max 32-bit inline value */
	struct header {
		uint8_t flags; // IN_USE, GC_MARK, DIRECT_VALUE, BITS_UNUSED; 4 more user defined
		uint8_t bytes_unused; // value size = node_size - bytes_unused (max=7)
		uint16_t datatype; // up to 64k user-definable datatypes
		union {
			uint32_t size; // 8-bit aligned
			uint32_t value; // if flagged as 'direct_value'
		}
	}
	#endif

For quick link traversal, the 'size' attribute contains the aligned size of the total entry. For this reason, another field is required to mark the 'unused' bytes. So for a 3-byte string, 'size' will be 8 (even or 16 to include the header) and bytes_unused will be 5 (or 11). If, however, the 'direct_value' flag is set, 'size' is implicit and zero (or 8) and the value is stored in the 'size' field instead. (You can still mark bytes_unused.)

The 64-bit aligned header is admittedly a bit roomy, but it does make the most sense for both 32-bit and 64-bit word sizes. Whenever size is an issue (over speed), the 32-bit aligned version may serve you better. Notice, though, that even here a 32-bit integer will take up 64 bits of total storage. So adjust your default integer size accordingly!

(As a kind of gimmick, I consider combining the extra room in the 64-bit aligned version of 'bytes_unused' with the BITS_UNUSED flag to mark non-byte-aligned sizes.)

As with malloc, the user will normally just receive a pointer and never see the header. But unlike malloc, the header information can be actively queried by the user, e.g.: size_of(ptr); type_of(ptr).


Node organisation
-----------------

Inintially, the heap contains only an end marker, which is a header with a zero allocation size. As new values are inserted, the end marker is moved to the new final position. Whenever a slot becomes available, the IN_USE flag is cleared. (But the slot is not zeroed out, if only to prevent confusion with the end marker: a clear slot will always have a size.) If it turns out that either the previous or the next slot (or both) are free as well, they will be merged into a single available space.

Whenever a new allocation is performed, initially tmmh will always try to re-use an available slot first. In case of multiple available slots, the slot with the best matching size is used(?) If this doesn't succeed, a new value will be inserted at the end of the list.


Pointer Identifying Functions
-----------------------------

On initialization, the user is requested to supply a 'pointer identifying function' ('pif') for each type stored. For simple types, suitable pifs are supplied. A pif has the following signature:

	bool pif_name (void * tmmh_data, int n, void ** result);

The simplest pif, pif_none, just returns false. It can be used for strings, integers and other values that do not hold pointers.

For any other structure, dynamic or static, which holds pointers to other tmmh-allocated values, the pif should fill in the value of the nth pointer and return true, unless there is no nth pointer.

The pifs are used both for marking values as reachable during garbage collection and for pointer relocation, e.g. during heap compression or when calling 'reallocate'.

Using this library
------------------

[See tmmh.h](tmmh.h)

