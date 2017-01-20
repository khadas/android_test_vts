/*
 * Copyright (c) 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.util;

import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;

/**
 * Helper object for describing graph data.
 */
public abstract class Graph {

    public static final String VALUE_KEY = "values";
    public static final String X_LABEL_KEY = "x_label";
    public static final String NAME_KEY = "name";
    public static final String TYPE_KEY = "type";

    public static enum GraphType {
        LINE_GRAPH,
        HISTOGRAM
    }

    /**
     * Get the graph type.
     * @return The graph type.
     */
    public abstract GraphType getType();

    /**
     * Get the x axis label.
     * @return The x axis label.
     */
    public abstract String getXLabel();

    /**
     * Get the name of the graph.
     * @return The name of the graph.
     */
    public abstract String getName();

    /**
     * Add data to the graph.
     * @param id The name of the graph.
     * @param profilingReport The profiling report message containing data to add.
     */
    public abstract void addData(String id, ProfilingReportMessage profilingReport);

    /**
     * Serializes the graph to json format.
     * @return A JsonElement object representing the graph object.
     */
    public JsonObject toJson() {
        JsonObject json = new JsonObject();
        json.add(X_LABEL_KEY, new JsonPrimitive(getXLabel()));
        json.add(NAME_KEY, new JsonPrimitive(getName()));
        json.add(TYPE_KEY, new JsonPrimitive(getType().toString()));
        return json;
    }
}

