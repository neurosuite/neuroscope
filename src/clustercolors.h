/***************************************************************************
                          clustercolors.h  -  description
                             -------------------
    begin                : Tue Sep 16 2004
    copyright            : (C) 2003 by Lynn Hazan
    email                : lynn.hazan@myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CLUSTERCOLORS_H
#define CLUSTERCOLORS_H

// include files for Qt
#include <QColor>
#include <QList>

// application specific includes
#include "itemcolors.h"

/**
  * This class represents the list of the clusters with their associated id, color
  * and color status (i.e the color has been changed or not).
  * The list index is zero based.
  *@author Lynn Hazan
  */

class ClusterColors : public ItemColors {

public:

    ClusterColors();
    virtual ~ClusterColors();
    explicit ClusterColors(const ClusterColors& clustercolors);

public:

    /**
  * Returns the color for a cluster with a given id or position in the list (@p identifier).
  * @param identifier depending on the mode the index in the list of clusters or the cluster idr
  * @param mode the way of looking up for the color
  * @return the QColor for the given cluster
  */
    virtual QColor color(int identifier, SelectionMode mode = ItemColors::BY_ITEM_NUMBER){return ItemColors::color(identifier,mode);}

    /**
  * Sets the color for a cluster with a given id or position in the list (@p identifier) to color.
  * @param identifier identifier depending on the mode the index in the list of clusters or the cluster id
  * @param mode the way of looking up for the cluster
  * @param color color to attribute to the cluster
  */
    virtual void setColor(int identifier, QColor color, SelectionMode mode = ItemColors::BY_ITEM_NUMBER){ItemColors::setColor(identifier,color,mode);}


    /**
  * Returns the cluster id with a given position in the list (@p index).
  * @param index the index in the list of clusters
  * @return the cluster id
  */
    virtual int clusterId(int index){return itemId(index);}

    /**
  * Returns true if the cluster Id exists false otherwise.
  * @param clusterId the id of the cluster to check for existence
  * @return the boolean value for the existance of the cluster
  */
    virtual bool contains(int clusterId){return ItemColors::contains(clusterId);}

    /**
  * Returns true if the color for a cluster with a given id or position in the list (@p identifier)
  * has been changed, otherwise returns false.
  * @param identifier depending on the mode the index in the list of clusters or the cluster id
  * @param mode the way of looking up for the color
  * @return the color status for the given cluster
  */
    virtual bool isColorChanged(int identifier, SelectionMode mode = ItemColors::BY_ITEM_NUMBER){return ItemColors::isColorChanged(identifier,mode);}

    /**
  * If changed is true, the color for a cluster with a given id or position in the list (@p identifier)
  * is said to have changed, otherwise is said not to have changed.
  * @param identifier depending on the mode the index in the list of clusters or the cluster id
  * @param mode the way of looking up for the color
  * @param changed color status.
  */
    virtual void setColorChanged(int identifier, bool changed, SelectionMode mode = ItemColors::BY_ITEM_NUMBER){
        ItemColors::setColorChanged(identifier,changed,mode);
    }

    /**
  * Returns the number of clusters.
  * @return the number of clusters in the list
  */
    uint numberOfClusters() const {return ItemColors::numberOfItems();}

    /**
  * Returns true if at least the color of one cluster have changed, otherwise returns false.
  * @return the color status for the identifierle list of clusters
  */
    virtual bool isColorChanged()const{return ItemColors::isColorChanged();}

    /**
  * Sets the color status for the entire list of clusters.
  * @param changed color status.
  */
    virtual void setColorChanged(bool changed){ItemColors::setColorChanged(changed);}


    /**
  * Appends a cluster to the list of clusters, the color status is set to false.
  * @param clusterId the cluster id.
  * @param color the color of the cluster.
  * @return the index in the list.
  */
    virtual uint append(int clusterId, QColor color){
        return ItemColors::append(clusterId,color);
    }

    /**
  * Inserts a cluster at position @p index in the list of clusters, the color status is set to false.
  * @param clusterId the cluster id.
  * @param index index position where to insert the cluster.
  * @param color the color of the cluster.
  */
    virtual void insert(int clusterId, QColor color,int index){
        ItemColors::insert(clusterId,color,index);
    }


    /**
  * Removes a cluster, with a given id or position in the list (@p identifier),from the list of clusters.
  * @param identifier depending on the mode the index in the list of clusters or the cluster id
  * @param mode the way of looking up for the cluster
  * @return true if successful,i.e. if identifier is in range, otherwise returns false.
  */
    virtual bool remove(int identifier, SelectionMode mode = ItemColors::BY_ITEM_NUMBER){
        return ItemColors::remove(identifier,mode);
    }

    /**
  * Returns the list of cluster ids for which the color has been changed since
  * the last reset of their status.
  * @return cluster ids list.
  */
    virtual QList<int> colorChangedClusterList(){
        return ItemColors::colorChangedItemList();
    }

    /**
  * Resets the status color of the object to false
  * and do the same for all the clusters
  */
    virtual void resetAllColorStatus(){
        ItemColors::resetAllColorStatus();
    }

    /**
  * Changes the clusterId of a given element in the list.
  * @param index position of the cluster in the list.
  * @param newClusterId the new id to assign.
  */
    virtual void changeClusterId(int index, int newClusterId){
        ItemColors::changeItemId(index,newClusterId);
    }
};

#endif
