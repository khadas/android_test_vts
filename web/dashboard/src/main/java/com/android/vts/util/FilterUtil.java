/**
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.vts.util;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.TestRunEntity;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.CompositeFilterOperator;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.FilterOperator;
import com.google.appengine.api.datastore.Query.FilterPredicate;
import com.google.gson.Gson;
import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.http.HttpServletRequest;

/** FilterUtil, a helper class for parsing and matching search queries to data. */
public class FilterUtil {
    protected static final Logger logger = Logger.getLogger(FilterUtil.class.getName());
    /** Key class to represent a filter token. */
    public enum FilterKey {
        DEVICE_BUILD_ID("deviceBuildId", DeviceInfoEntity.BUILD_ID, true),
        BRANCH("branch", DeviceInfoEntity.BRANCH, true),
        TARGET("device", DeviceInfoEntity.BUILD_FLAVOR, true),
        VTS_BUILD_ID("testBuildId", TestRunEntity.TEST_BUILD_ID, false),
        HOSTNAME("hostname", TestRunEntity.HOST_NAME, false),
        PASSING("passing", TestRunEntity.PASS_COUNT, false),
        NONPASSING("nonpassing", TestRunEntity.FAIL_COUNT, false);

        private static final Map<String, FilterKey> keyMap;

        static {
            keyMap = new HashMap<>();
            for (FilterKey k : EnumSet.allOf(FilterKey.class)) {
                keyMap.put(k.keyString, k);
            }
        }

        /**
         * Test if a string is a valid device key.
         *
         * @param keyString The key string.
         * @return True if they key string matches a key and the key is a device filter.
         */
        public static boolean isDeviceKey(String keyString) {
            return keyMap.containsKey(keyString) && keyMap.get(keyString).isDevice;
        }

        /**
         * Test if a string is a valid test key.
         *
         * @param keyString The key string.
         * @return True if they key string matches a key and the key is a test filter.
         */
        public static boolean isTestKey(String keyString) {
            return keyMap.containsKey(keyString) && !keyMap.get(keyString).isDevice;
        }

        /**
         * Parses a key string into a key.
         *
         * @param keyString The key string.
         * @return The key matching the key string.
         */
        public static FilterKey parse(String keyString) {
            return keyMap.get(keyString);
        }

        private final String keyString;
        private final String property;
        private final boolean isDevice;

        /**
         * Constructs a key with the specified key string.
         *
         * @param keyString The identifying key string.
         * @param propertyName The name of the property to match.
         */
        private FilterKey(String keyString, String propertyName, boolean isDevice) {
            this.keyString = keyString;
            this.property = propertyName;
            this.isDevice = isDevice;
        }

        public Filter getFilterForString(String matchString) {
            return new FilterPredicate(this.property, FilterOperator.EQUAL, matchString);
        }

        public Filter getFilterForNumber(long matchNumber) {
            return new FilterPredicate(this.property, FilterOperator.EQUAL, matchNumber);
        }
    }

    /**
     * Get a filter on devices from a user search query.
     *
     * @param parameterMap The key-value map of url parameters.
     * @return A filter with the values from the user search parameters.
     */
    public static Filter getUserDeviceFilter(Map<String, Object> parameterMap) {
        Filter deviceFilter = null;
        for (String key : parameterMap.keySet()) {
            if (!FilterKey.isDeviceKey(key))
                continue;
            String[] values = (String[]) parameterMap.get(key);
            if (values.length == 0)
                continue;
            String value = values[0];
            FilterKey filterKey = FilterKey.parse(key);
            Filter f = filterKey.getFilterForString(value);
            if (deviceFilter == null) {
                deviceFilter = f;
            } else {
                deviceFilter = CompositeFilterOperator.and(deviceFilter, f);
            }
        }
        return deviceFilter;
    }

    /**
     * Get a filter on test runs from a user search query.
     *
     * @param parameterMap The key-value map of url parameters.
     * @return A filter with the values from the user search parameters.
     */
    public static Filter getUserTestFilter(Map<String, Object> parameterMap) {
        Filter testFilter = null;
        for (String key : parameterMap.keySet()) {
            if (!FilterKey.isTestKey(key))
                continue;
            String[] values = (String[]) parameterMap.get(key);
            if (values.length == 0)
                continue;
            String stringValue = values[0];
            FilterKey filterKey = FilterKey.parse(key);
            Filter f = null;
            switch (filterKey) {
                case NONPASSING:
                case PASSING:
                    try {
                        Long value = Long.parseLong(stringValue);
                        f = filterKey.getFilterForNumber(value);
                    } catch (NumberFormatException e) {
                        // invalid number
                    }
                    break;
                case HOSTNAME:
                case VTS_BUILD_ID:
                    f = filterKey.getFilterForString(stringValue.toLowerCase());
                    break;
                default:
                    break;
            }
            if (testFilter == null) {
                testFilter = f;
            } else if (f != null) {
                testFilter = CompositeFilterOperator.and(testFilter, f);
            }
        }
        return testFilter;
    }

    /**
     * Get a filter on the test run type.
     *
     * @param showPresubmit True to display presubmit tests.
     * @param showPostsubmit True to display postsubmit tests.
     * @param unfiltered True if no filtering should be applied.
     * @return A filter on the test type.
     */
    public static Filter getTestTypeFilter(
            boolean showPresubmit, boolean showPostsubmit, boolean unfiltered) {
        if (unfiltered) {
            return null;
        } else if (showPresubmit && !showPostsubmit) {
            return new FilterPredicate(TestRunEntity.TYPE, FilterOperator.EQUAL,
                    TestRunEntity.TestRunType.PRESUBMIT.getNumber());
        } else if (showPostsubmit && !showPresubmit) {
            return new FilterPredicate(TestRunEntity.TYPE, FilterOperator.EQUAL,
                    TestRunEntity.TestRunType.POSTSUBMIT.getNumber());
        } else {
            List<Integer> types = new ArrayList<>();
            types.add(TestRunEntity.TestRunType.PRESUBMIT.getNumber());
            types.add(TestRunEntity.TestRunType.POSTSUBMIT.getNumber());
            return new FilterPredicate(TestRunEntity.TYPE, FilterOperator.IN, types);
        }
    }

    /**
     * Get the time range filter to apply to a query.
     *
     * @param testKey The key of the parent TestEntity object.
     * @param kind The kind to use for the filters.
     * @param startTime The start time in microseconds, or null if unbounded.
     * @param endTime The end time in microseconds, or null if unbounded.
     * @param testRunFilter The existing filter on test runs to apply, or null.
     * @return A filter to apply on test runs.
     */
    public static Filter getTimeFilter(
            Key testKey, String kind, Long startTime, Long endTime, Filter testRunFilter) {
        if (startTime == null && endTime == null) {
            endTime = TimeUnit.MILLISECONDS.toMicros(System.currentTimeMillis());
        }

        Filter startFilter = null;
        Filter endFilter = null;
        Filter filter = null;
        if (startTime != null) {
            Key startKey = KeyFactory.createKey(testKey, kind, startTime);
            startFilter = new FilterPredicate(
                    Entity.KEY_RESERVED_PROPERTY, FilterOperator.GREATER_THAN_OR_EQUAL, startKey);
            filter = startFilter;
        }
        if (endTime != null) {
            Key endKey = KeyFactory.createKey(testKey, kind, endTime);
            endFilter = new FilterPredicate(
                    Entity.KEY_RESERVED_PROPERTY, FilterOperator.LESS_THAN_OR_EQUAL, endKey);
            filter = endFilter;
        }
        if (startFilter != null && endFilter != null) {
            filter = CompositeFilterOperator.and(startFilter, endFilter);
        }
        if (testRunFilter != null) {
            filter = CompositeFilterOperator.and(filter, testRunFilter);
        }
        return filter;
    }

    public static Filter getTimeFilter(Key testKey, String kind, Long startTime, Long endTime) {
        return getTimeFilter(testKey, kind, startTime, endTime, null);
    }

    /**
     * Get the list of keys matching the provided test filter and device filter.
     *
     * @param ancestorKey The ancestor key to use in the query.
     * @param kind The entity kind to use in the test query.
     * @param testFilter The filter to apply to the test runs.
     * @param deviceFilter The filter to apply to associated devices.
     * @param dir The sort direction of the returned list.
     * @param maxSize The maximum number of entities to return.
     * @return a list of keys matching the provided test and device filters.
     */
    public static List<Key> getMatchingKeys(Key ancestorKey, String kind, Filter testFilter,
            Filter deviceFilter, Query.SortDirection dir, int maxSize) {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Set<Key> matchingTestKeys = new HashSet<>();
        Query testQuery =
                new Query(kind).setAncestor(ancestorKey).setFilter(testFilter).setKeysOnly();
        for (Entity testRunKey : datastore.prepare(testQuery).asIterable()) {
            matchingTestKeys.add(testRunKey.getKey());
        }

        Set<Key> allMatchingKeys;
        if (deviceFilter == null) {
            allMatchingKeys = matchingTestKeys;
        } else {
            allMatchingKeys = new HashSet<>();
            Query deviceQuery = new Query(DeviceInfoEntity.KIND)
                                        .setAncestor(ancestorKey)
                                        .setFilter(deviceFilter)
                                        .setKeysOnly();
            for (Entity device : datastore.prepare(deviceQuery).asIterable()) {
                if (matchingTestKeys.contains(device.getKey().getParent())) {
                    allMatchingKeys.add(device.getKey().getParent());
                }
            }
        }
        List<Key> gets = new ArrayList<>(allMatchingKeys);
        if (dir == Query.SortDirection.DESCENDING) {
            Collections.sort(gets, Collections.reverseOrder());
        } else {
            Collections.sort(gets);
        }
        gets = gets.subList(0, Math.min(gets.size(), maxSize));
        return gets;
    }

    /**
     * Set the request with the provided key/value attribute map.
     * @param request The request whose attributes to set.
     * @param parameterMap The map from key to (Object) String[] value whose entries to parse.
     */
    public static void setAttributes(HttpServletRequest request, Map<String, Object> parameterMap) {
        for (String key : parameterMap.keySet()) {
            if (!FilterKey.isDeviceKey(key) && !FilterKey.isTestKey(key))
                continue;
            FilterKey filterKey = FilterKey.parse(key);
            String[] values = (String[]) parameterMap.get(key);
            if (values.length == 0)
                continue;
            String stringValue = values[0];
            request.setAttribute(filterKey.keyString, new Gson().toJson(stringValue));
        }
    }
}
