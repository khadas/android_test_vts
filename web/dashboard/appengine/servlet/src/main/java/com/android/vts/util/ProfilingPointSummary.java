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
import com.android.vts.util.StatSummary;
import com.google.protobuf.ByteString;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Represents statistical summaries of each profiling point.
 */
public class ProfilingPointSummary implements Iterable<StatSummary> {
    private List<StatSummary> statSummaries;
    private Map<ByteString, Integer> labelIndices;
    private List<ByteString> labels;
    public ByteString xLabel;
    public ByteString yLabel;

    /**
     * Initializes the summary with empty arrays.
     */
    public ProfilingPointSummary() {
        statSummaries = new ArrayList<>();
        labelIndices = new HashMap<>();
        labels = null;
    }

    /**
     * Determines if a data label is present in the profiling point.
     * @param label The name of the label.
     * @return true if the label is present, false otherwise.
     */
    public boolean hasLabel(ByteString label) {
        return labelIndices.containsKey(label);
    }

    /**
     * Gets the statistical summary for a specified data label.
     * @param label The name of the label.
     * @return The StatSummary object if it exists, or null otherwise.
     */
    public StatSummary getStatSummary(ByteString label) {
        if (!hasLabel(label)) return null;
        return statSummaries.get(labelIndices.get(label));
    }

    /**
     * Updates the profiling summary with the data from a new profiling report.
     * @param report The ProfilingReportMessage object containing profiling data.
     */
    public void update(ProfilingReportMessage report) {
        for (int i = 0; i < report.getLabelList().size(); i++) {
            ByteString label = report.getLabelList().get(i);
            if (!labelIndices.containsKey(label)) {
                StatSummary summary = new StatSummary(label);
                labelIndices.put(label, statSummaries.size());
                statSummaries.add(summary);
            }
            StatSummary summary = getStatSummary(label);
            summary.updateStats(report.getValueList().get(i));
        }
        this.labels = report.getLabelList();
        this.xLabel = report.getXAxisLabel();
        this.yLabel = report.getYAxisLabel();
    }


    /**
     * Gets an iterator that returns stat summaries in the ordered the labels were specified in the
     * ProfilingReportMessage objects.
     */
    @Override
    public Iterator<StatSummary> iterator() {
        Iterator<StatSummary> it = new Iterator<StatSummary>() {
            private int currentIndex = 0;

            @Override
            public boolean hasNext() {
                return labels != null && currentIndex < labels.size();
            }

            @Override
            public StatSummary next() {
                ByteString label = labels.get(currentIndex++);
                return statSummaries.get(labelIndices.get(label));
            }
        };
        return it;
    }
}
