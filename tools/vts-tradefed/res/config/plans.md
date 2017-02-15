# VTS Test Plans

A VTS test plan consists of a set of VTS tests. Typically the tests within a
plan are for the same target component or testing the same or similar aspect
(e.g., functionality, performance, robustness, or power). There are two kinds
of plans in VTS:

## 1. Official Plans

Official plans contain only verified tests, and are for all Android developers
and partners.

 * vts: For all default VTS tests.
 * vts-hal: For all default VTS HAL (hardware abstraction layer) module tests.
 * vts-kernel: For all default VTS kernel tests.
 * vts-library: For all default VTS library tests.
 * vts-fuzz: For all default VTS fuzz tests.
 * vts-performance: For all default VTS performance tests.
 * vts-hal-hidl: For all default VTS HIDL (Hardware Interface Definition Language)
 HAL tests.
 * vts-hal-hidl-profiling: For all default VTS HIDL HAL performance profiling tests.

## 2. Serving Plans

Serving plans contain both verified and experimental tests, and are for Android
partners to use as part of their continuous integration infrastructure. The
name of a serving plan always starts with 'vts-serving-'.