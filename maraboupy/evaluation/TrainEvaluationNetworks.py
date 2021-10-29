import tensorflow as tf
from tensorflow.keras import datasets, layers, models


def createNet(netName):

    if netName == 'evalNetA':
        return tf.keras.Sequential(
            [
                tf.keras.Input(shape=self.ds.input_shape),
                layers.Conv2D(1, kernel_size=(3, 3), activation="relu", name="c1"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                layers.Conv2D(1, kernel_size=(2, 2), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dense(40, activation="relu", name="fc1"),
                layers.Dense(self.ds.num_classes, activation=None, name="sm1"),
            ],
            name=netName
        )        
    elif netName == 'evalNetB':
        return tf.keras.Sequential(
            [
                tf.keras.Input(shape=self.ds.input_shape),
                layers.Conv2D(2, kernel_size=(3, 3), activation="relu", name="c1"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                layers.Conv2D(2, kernel_size=(2, 2), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dense(40, activation="relu", name="fc1"),
                layers.Dense(self.ds.num_classes, activation=None, name="sm1"),
            ],
            name=netName
        )
    elif netName == 'evalNetC':
        return  tf.keras.Sequential(
            [
                tf.keras.Input(shape=self.ds.input_shape),
                layers.Conv2D(2, kernel_size=(3, 3), activation="relu", name="c0"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp0"),
                layers.Conv2D(2, kernel_size=(2, 2), activation="relu", name="c1"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                layers.Conv2D(2, kernel_size=(3, 3), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dense(40, activation="relu", name="fc1"),
                layers.Dense(self.ds.num_classes, activation=None, name="sm1"),
            ],
            name=netName
        )
    else :
        raise NotImplementedError


def train(netName, cfg):
    if not hasattr(cfg, ds):
        #FIXME add for all attributes.
        
    net = createNet(netName)
    batch_size = 128
    epochs = 15    
    net.build(input_shape=self.ds.featureShape)
    net.summary()
    net.compile(optimizer=self.ds.optimizer, loss=myLoss, metrics=[tf.keras.metrics.SparseCategoricalAccuracy()])
    net.fit(self.ds.x_train, self.ds.y_train, epochs=epochs, batch_size=batch_size, validation_split=0.1)
    score = net.evaluate(self.ds.x_test, self.ds.y_test, verbose=0)
    CnnAbs.printLog("(Original) Test loss:{}".format(score[0]))
    CnnAbs.printLog("(Original) Test accuracy:{}".format(score[1]))
    net.save(netName + '.h5')
else:
    net = load_model(basePath + "/" + savedModelOrig, custom_objects={'myLoss': myLoss})
            net.summary()
            score = net.evaluate(self.ds.x_test, self.ds.y_test, verbose=0)
            CnnAbs.printLog("(Original) Test loss:{}".format(score[0]))
            CnnAbs.printLog("(Original) Test accuracy:{}".format(score[1]))
    
        return net    
